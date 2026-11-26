#!/usr/bin/env python3
"""Focused installed-release check: real input/audio/display, isolated settings.

Run on CardputerZero as its normal desktop user after installing the .deb.
The launcher is paused for display ownership and always restored on completion.
"""
from array import array
import fcntl
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[1]
version = sys.argv[1] if len(sys.argv) > 1 else (root/'VERSION.txt').read_text().strip()
out = root/'build/installed-acceptance'
out.mkdir(parents=True, exist_ok=True)
app = Path('/usr/share/APPLaunch')
binary = app/'bin/M5CardputerZero-ZeroChord'
wrapper = app/'bin/run-zerochord'
assert subprocess.check_output(['dpkg-query','-W','-f=${Version} ${Status}','zerochord'], text=True) == version+' install ok installed'
assert subprocess.check_output([str(wrapper),'--version'], text=True).strip() == 'ZeroChord '+version
assert subprocess.check_output(['dpkg','--verify','zerochord'], text=True) == ''
metadata = dict(line.split('=',1) for line in (app/'applications/zerochord.desktop').read_text().splitlines() if '=' in line)
assert metadata['Name'] == 'ZeroChord' and metadata['Exec'] == 'bin/run-zerochord'
assert metadata['Terminal'] == 'false' and metadata['Icon'] == 'share/images/zerochord.png'
assert struct.unpack_from('>II',(app/metadata['Icon']).read_bytes(),16) == (100,100)
fd = os.open('/dev/uinput',os.O_WRONLY|os.O_NONBLOCK)
created = False
process = monitor = recording = None
launcher_stopped = False

def emit(code, down):
    os.write(fd, struct.pack('llHHi',0,0,1,code,int(down)))
    os.write(fd, struct.pack('llHHi',0,0,0,0,0))

def tap(code):
    emit(code,True); time.sleep(.035); emit(code,False); time.sleep(.07)

def framebuffer(path):
    rows = Path('/proc/fb').read_text().splitlines()
    match = next(row.split()[0] for row in rows if 'panel-mipi' in row or 'st7789' in row)
    with open('/dev/fb'+match,'rb',buffering=0) as fb:
        info=bytearray(160); fcntl.ioctl(fb.fileno(),0x4600,info)
        v=struct.unpack_from('=40I',info)
        width,height,_,_,xoff,yoff,bpp=v[:7]
        assert (width,height,bpp)==(320,170,16)
        stride=int(Path('/sys/class/graphics/fb'+match+'/stride').read_text())
        raw=fb.read(stride*(height+yoff))
    rgb=bytearray()
    for y in range(height):
        for x in range(width):
            at=(y+yoff)*stride+(x+xoff)*2
            pixel=int.from_bytes(raw[at:at+2],'little')
            for shift,bits in [(v[8],v[9]),(v[11],v[12]),(v[14],v[15])]:
                rgb.append(((pixel>>shift)&((1<<bits)-1))*255//((1<<bits)-1))
    assert len(set(rgb))>16
    path.write_bytes(b'P6\n320 170\n255\n'+rgb)
    return rgb

try:
    fcntl.ioctl(fd,0x40045564,1)
    for code in range(1,256): fcntl.ioctl(fd,0x40045565,code)
    name='ZeroChord installed release test'
    os.write(fd,struct.pack('80sHHHHI',name.encode(),3,0x1209,1,1,0)+bytes(1024))
    fcntl.ioctl(fd,0x5501); created=True
    device=None
    for attempt in range(40):
        for path in Path('/sys/class/input').glob('event*'):
            if (path/'device/name').read_text().strip()==name:
                device='/dev/input/'+path.name
        if device and os.access(device,os.R_OK): break
        time.sleep(.1)
    assert device and os.access(device,os.R_OK)
    config=out/('config-'+str(os.getpid()))
    environment=dict(os.environ,ZEROCHORD_INPUT_DEVICE=device,XDG_CONFIG_HOME=str(config))
    subprocess.run(['systemctl','--user','stop','zerochord-preview.service'],check=False)
    subprocess.run(['systemctl','--user','stop','APPLaunch.service'],check=True)
    launcher_stopped=True
    backlight=Path('/sys/class/backlight/backlight/brightness')
    if int(backlight.read_text())==0: backlight.write_text('80\n')
    sink=subprocess.check_output(['pactl','get-default-sink'],text=True).strip()
    recording=(out/'output.s16le').open('wb')
    monitor=subprocess.Popen(['parec','--device='+sink+'.monitor','--format=s16le',
                              '--rate=48000','--channels=2','--latency-msec=50'],
                             stdout=recording,stderr=subprocess.DEVNULL)
    with (out/'installed.log').open('w') as log:
        process=subprocess.Popen([str(wrapper),'--capture',str(out/'app.ppm')],
                                 cwd='/usr/share/APPLaunch',env=environment,stdout=log,stderr=log)
        time.sleep(.9)
        assert process.poll() is None and Path('/proc/'+str(process.pid)+'/exe').resolve()==binary
        tap(9) # 8: chord
        for code in [44,45,46,47,48,49,50,38]: tap(code) # Z-M and L
        time.sleep(.3)
        tap(24) # O: Backing layers, Layers tab; all layers start on
        tap(45); tap(28); tap(45); tap(28) # Mute Arp and Pad: Chord, Bass and Drums stay on
        tap(1) # Esc: back to playing
        tap(15) # Tab: settings, Style
        tap(45); tap(45); tap(28) # X X -> Backing layers, Enter
        tap(3) # 2: Chord tab, Pattern
        tap(28); tap(45); tap(28) # Enter, X, Enter: the second pattern in the list
        tap(1); tap(1) # Esc Esc: performance
        tap(9); time.sleep(.9)
        # App > Volume & effects, using the printed Z/C/X arrows.
        tap(15); tap(15) # Tab: settings, then Tab: App
        tap(45); tap(28)
        tap(45); tap(44) # App Vol: 100% to 95%.
        for _ in range(4): tap(45) # App Vol to EQ Low.
        for _ in range(3): tap(46)
        tap(1); tap(1)
        tap(25) # P: numeric tempo
        for code in [2,4,8,28]: tap(code) # 137 Enter
        tap(9); time.sleep(.3)
        kernel=framebuffer(out/'kernel.ppm')
        assert int(backlight.read_text())>0
        emit(1,True); time.sleep(.95); emit(1,False)
        assert process.wait(timeout=5)==0
    settings=dict(line.split(maxsplit=1) for line in (config/'zerochord/settings.conf').read_text().splitlines())
    expected = {'masterVolume':'95', 'eqLowDb':'3', 'mode':'1', 'chordPattern':'1', 'bpm':'137',
                'bass':'1', 'drums':'1'}
    assert {key: settings[key] for key in expected} == expected, settings
    internal=(out/'app.ppm').read_bytes().split(b'\n',3)[3]
    assert len(kernel)==len(internal) and max(abs(a-b) for a,b in zip(kernel,internal))<=8
    log=(out/'installed.log').read_text()
    assert 'queue_overflows=0' in log and 'Audio callbacks=' in log, log
    monitor.terminate(); monitor.wait(timeout=5); monitor=None
    recording.close(); recording=None
    values=array('h'); values.frombytes((out/'output.s16le').read_bytes())
    assert len(values)>48000 and max(abs(v) for v in values)>100
    subprocess.run([str(wrapper),'--no-save','--quit-after-ms','300'],cwd='/usr/share/APPLaunch',
                   env=environment,check=True,timeout=6,stdout=subprocess.DEVNULL)
    result={'version':version,'status':'passed','installed_wrapper':str(wrapper),
            'audio_frames':len(values)//2,'audio_peak':max(abs(v) for v in values),
            'input':'temporary evdev keyboard; printed Z/C/F/X navigation',
            'settings':'isolated; Chord-only layer, chord pattern, App Vol, EQ and tempo saved/reloaded',
            'framebuffer':'320x170 kernel readback matches app capture within RGB565 quantization',
            'exit':'long Esc exits cleanly; launcher restored in finally',
            'limits':'Digital audio and injected keys; physical feel and listening remain manual.'}
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
finally:
    if process and process.poll() is None:
        process.terminate(); process.wait(timeout=5)
    if monitor and monitor.poll() is None:
        monitor.terminate(); monitor.wait(timeout=5)
    if recording: recording.close()
    if created: fcntl.ioctl(fd,0x5502)
    os.close(fd)
    if launcher_stopped:
        subprocess.run(['systemctl','--user','start','APPLaunch.service'],check=True)
