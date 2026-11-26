#!/usr/bin/env python3
"""Copy selected presets from a SoundFont 2 bank into a smaller, byte-deterministic SF2.

Only the chosen presets, the instruments they reference (generator 41), the samples
those reference (generator 53) and any stereo-linked partner samples are kept. Sample
data is copied unchanged; nothing is resampled or re-encoded.
"""
import argparse
import struct
import sys
from pathlib import Path

INSTRUMENT = 41
SAMPLE_ID = 53
ZERO_PADDING = 46  # Zero-valued sample points required after each sample (SF2.04 7.10).
LINKED_TYPES = {2, 4, 8}  # Right, left and linked samples; ROM samples (0x8000) are rejected.

PHDR = struct.Struct('<20sHHHIII')
BAG = struct.Struct('<HH')
MOD = struct.Struct('<HHhHH')
GEN = struct.Struct('<HH')
INST = struct.Struct('<20sH')
SHDR = struct.Struct('<20sIIIIIBbHH')


class SoundFontError(ValueError):
    pass


def _chunks(data, start, end):
    """Yield (id, payload_start, payload_end) for consecutive RIFF chunks."""
    pos = start
    while pos + 8 <= end:
        cid, size = struct.unpack_from('<4sI', data, pos)
        body = pos + 8
        if body + size > end:
            raise SoundFontError(f'Chunk {cid!r} exceeds its container')
        yield cid, body, body + size
        pos = body + size + (size & 1)
    if pos != end and pos != end + 1:
        raise SoundFontError('Trailing bytes after the last chunk')


def _records(data, struct_type, name):
    if len(data) % struct_type.size or len(data) < struct_type.size:
        raise SoundFontError(f'Malformed {name} chunk')
    return [struct_type.unpack_from(data, n) for n in range(0, len(data), struct_type.size)]


def _name(raw):
    return raw.split(b'\0', 1)[0].decode('latin-1')


class SoundFont:
    """Parsed SF2 structure; sample data stays as memoryviews into the source bytes."""

    def __init__(self, data):
        data = memoryview(data)
        if len(data) < 12 or bytes(data[:4]) != b'RIFF' or bytes(data[8:12]) != b'sfbk':
            raise SoundFontError('Not a RIFF sfbk file')
        riff_end = 8 + struct.unpack_from('<I', data, 4)[0]
        if riff_end > len(data):
            raise SoundFontError('Truncated RIFF file')
        lists = {}
        for cid, body, end in _chunks(data, 12, riff_end):
            if cid == b'LIST':
                kind = bytes(data[body:body + 4])
                lists[kind] = {c: data[b:e] for c, b, e in _chunks(data, body + 4, end)}
                lists[kind + b'-order'] = [c for c, _, _ in _chunks(data, body + 4, end)]
        for kind in (b'INFO', b'sdta', b'pdta'):
            if kind not in lists:
                raise SoundFontError(f'Missing LIST {kind.decode()}')
        self.info = [(c, bytes(lists[b'INFO'][c])) for c in lists[b'INFO-order']]
        self.smpl = lists[b'sdta'].get(b'smpl')
        self.sm24 = lists[b'sdta'].get(b'sm24')
        if self.smpl is None or len(self.smpl) % 2:
            raise SoundFontError('Missing or malformed smpl chunk')
        pdta = lists[b'pdta']
        self.phdr = _records(pdta[b'phdr'], PHDR, 'phdr')
        self.pbag = _records(pdta[b'pbag'], BAG, 'pbag')
        self.pmod = _records(pdta[b'pmod'], MOD, 'pmod')
        self.pgen = _records(pdta[b'pgen'], GEN, 'pgen')
        self.inst = _records(pdta[b'inst'], INST, 'inst')
        self.ibag = _records(pdta[b'ibag'], BAG, 'ibag')
        self.imod = _records(pdta[b'imod'], MOD, 'imod')
        self.igen = _records(pdta[b'igen'], GEN, 'igen')
        self.shdr = _records(pdta[b'shdr'], SHDR, 'shdr')

    def presets(self):
        """Return (bank, program, name, index) for every preset except the terminal EOP."""
        return [(p[2], p[1], _name(p[0]), n) for n, p in enumerate(self.phdr[:-1])]

    # Zones of record n are bags[header[n].bag : header[n+1].bag]; each bag's generators
    # are gens[bag.gen : nextbag.gen] and modulators mods[bag.mod : nextbag.mod].
    @staticmethod
    def _zones(headers, bag_field, bags, n):
        first, last = headers[n][bag_field], headers[n + 1][bag_field]
        if not first <= last < len(bags):
            raise SoundFontError('Bag index out of range')
        return range(first, last)

    @staticmethod
    def _span(bags, b, field, records):
        first, last = bags[b][field], bags[b + 1][field]
        if not first <= last < len(records):
            raise SoundFontError('Generator or modulator index out of range')
        return range(first, last)


def _info_chunk(cid, text):
    raw = text.encode('latin-1') + b'\0'
    if len(raw) & 1:
        raw += b'\0'
    return cid, raw


def _riff(cid, payload):
    return struct.pack('<4sI', cid, len(payload)) + payload + (b'\0' if len(payload) & 1 else b'')


def _list(kind, chunks):
    return _riff(b'LIST', kind + b''.join(_riff(c, p) for c, p in chunks))


def subset(sf, wanted, info=None):
    """Return SF2 bytes containing exactly the (bank, program) presets in `wanted`.

    `info` maps INFO chunk ids (such as b'INAM') to replacement text; other INFO chunks
    are copied unchanged and new ids are appended.
    """
    index = {(bank, program): n for bank, program, _, n in sf.presets()}
    missing = [w for w in wanted if w not in index]
    if missing:
        raise SoundFontError(f'Presets not found: {missing}')
    if len(set(wanted)) != len(wanted):
        raise SoundFontError('Duplicate presets requested')
    presets = sorted(index[w] for w in wanted)  # Keep the source order.

    # Instruments referenced by the chosen presets.
    instruments = set()
    for p in presets:
        for b in sf._zones(sf.phdr, 3, sf.pbag, p):
            for g in sf._span(sf.pbag, b, 0, sf.pgen):
                if sf.pgen[g][0] == INSTRUMENT:
                    if sf.pgen[g][1] >= len(sf.inst) - 1:
                        raise SoundFontError('Instrument index out of range')
                    instruments.add(sf.pgen[g][1])
    instruments = sorted(instruments)

    # Samples referenced by those instruments, plus stereo partners (transitively).
    samples = set()
    for i in instruments:
        for b in sf._zones(sf.inst, 1, sf.ibag, i):
            for g in sf._span(sf.ibag, b, 0, sf.igen):
                if sf.igen[g][0] == SAMPLE_ID:
                    samples.add(sf.igen[g][1])
    pending = list(samples)
    while pending:
        s = pending.pop()
        if s >= len(sf.shdr) - 1:
            raise SoundFontError('Sample index out of range')
        sample_type, link = sf.shdr[s][9], sf.shdr[s][8]
        if sample_type & 0x8000:
            raise SoundFontError('ROM samples are not supported')
        if sample_type in LINKED_TYPES and link not in samples:
            samples.add(link)
            pending.append(link)
    samples = sorted(samples)
    new_inst = {old: new for new, old in enumerate(instruments)}
    new_sample = {old: new for new, old in enumerate(samples)}

    # Sample data: each sample's points, then 46 zero points; headers rebased.
    smpl, sm24, shdr = bytearray(), bytearray(), []
    total_points = len(sf.smpl) // 2
    for s in samples:
        name_raw, start, end, loop_start, loop_end, rate, pitch, correction, link, kind = sf.shdr[s]
        if not start <= end <= total_points:
            raise SoundFontError(f'Sample {_name(name_raw)!r} lies outside smpl')
        if not start <= loop_start <= loop_end <= end:
            raise SoundFontError(f'Sample {_name(name_raw)!r} loop lies outside the sample')
        base = len(smpl) // 2
        smpl += sf.smpl[start * 2:end * 2]
        smpl += bytes(ZERO_PADDING * 2)
        if sf.sm24 is not None:
            sm24 += sf.sm24[start:end]
            sm24 += bytes(ZERO_PADDING)
        if kind in LINKED_TYPES:
            link = new_sample[link]
        shift = base - start
        shdr.append((name_raw, start + shift, end + shift, loop_start + shift, loop_end + shift,
                     rate, pitch, correction, link, kind))
    shdr.append((b'EOS'.ljust(20, b'\0'), 0, 0, 0, 0, 0, 0, 0, 0, 0))

    def copy_zones(headers, bag_field, bags, gens, mods, chosen, ref_gen, remap):
        out_bags, out_gens, out_mods = [], [], []
        starts = []
        for h in chosen:
            starts.append(len(out_bags))
            for b in SoundFont._zones(headers, bag_field, bags, h):
                out_bags.append((len(out_gens), len(out_mods)))
                for g in SoundFont._span(bags, b, 0, gens):
                    oper, amount = gens[g]
                    out_gens.append((oper, remap[amount] if oper == ref_gen else amount))
                out_mods.extend(mods[m] for m in SoundFont._span(bags, b, 1, mods))
        out_bags.append((len(out_gens), len(out_mods)))
        out_gens.append((0, 0))
        out_mods.append((0, 0, 0, 0, 0))
        return starts, out_bags, out_gens, out_mods

    p_starts, pbag, pgen, pmod = copy_zones(sf.phdr, 3, sf.pbag, sf.pgen, sf.pmod, presets,
                                            INSTRUMENT, new_inst)
    i_starts, ibag, igen, imod = copy_zones(sf.inst, 1, sf.ibag, sf.igen, sf.imod, instruments,
                                            SAMPLE_ID, new_sample)
    phdr = [sf.phdr[p][:3] + (start,) + sf.phdr[p][4:] for p, start in zip(presets, p_starts)]
    phdr.append((b'EOP'.ljust(20, b'\0'), 0, 0, len(pbag) - 1, 0, 0, 0))
    inst = [(sf.inst[i][0], start) for i, start in zip(instruments, i_starts)]
    inst.append((b'EOI'.ljust(20, b'\0'), len(ibag) - 1))
    for count, what in ((len(pgen), 'preset generators'), (len(pmod), 'preset modulators'),
                        (len(igen), 'instrument generators'), (len(imod), 'instrument modulators'),
                        (len(pbag), 'preset zones'), (len(ibag), 'instrument zones')):
        if count > 0xFFFF:
            raise SoundFontError(f'Too many {what} for SF2 16-bit indices')

    overrides = dict(info or {})
    info = [_info_chunk(cid, overrides.pop(cid)) if cid in overrides else (cid, payload)
            for cid, payload in sf.info]
    info += [_info_chunk(cid, text) for cid, text in overrides.items()]

    def pack(struct_type, rows):
        return b''.join(struct_type.pack(*row) for row in rows)

    sdta = [(b'smpl', bytes(smpl))] + ([(b'sm24', bytes(sm24))] if sf.sm24 is not None else [])
    pdta = [(b'phdr', pack(PHDR, phdr)), (b'pbag', pack(BAG, pbag)), (b'pmod', pack(MOD, pmod)),
            (b'pgen', pack(GEN, pgen)), (b'inst', pack(INST, inst)), (b'ibag', pack(BAG, ibag)),
            (b'imod', pack(MOD, imod)), (b'igen', pack(GEN, igen)), (b'shdr', pack(SHDR, shdr))]
    body = b'sfbk' + _list(b'INFO', info) + _list(b'sdta', sdta) + _list(b'pdta', pdta)
    return _riff(b'RIFF', body)


def check(data, wanted):
    """Re-parse a subset and check its structure; return the parsed SoundFont."""
    sf = SoundFont(data)
    if sorted((b, p) for b, p, _, _ in sf.presets()) != sorted(wanted):
        raise SoundFontError('Subset presets differ from the request')
    for headers, field, bags, gens, ref, limit in (
            (sf.phdr, 3, sf.pbag, sf.pgen, INSTRUMENT, len(sf.inst) - 1),
            (sf.inst, 1, sf.ibag, sf.igen, SAMPLE_ID, len(sf.shdr) - 1)):
        if headers[-1][field] != len(bags) - 1:
            raise SoundFontError('Terminal record does not close the zone list')
        if bags[-1] != (len(gens) - 1, bags[-1][1]) or gens[-1] != (0, 0):
            raise SoundFontError('Terminal zone or generator is wrong')
        for n in range(len(headers) - 1):
            for b in SoundFont._zones(headers, field, bags, n):
                for g in SoundFont._span(bags, b, 0, gens):
                    if gens[g][0] == ref and gens[g][1] >= limit:
                        raise SoundFontError('Dangling instrument or sample reference')
    points = len(sf.smpl) // 2
    for _, start, end, loop_start, loop_end, *_rest, link, kind in sf.shdr[:-1]:
        if not start <= loop_start <= loop_end <= end <= points - ZERO_PADDING:
            raise SoundFontError('Sample header outside smpl')
        if any(sf.smpl[end * 2:(end + ZERO_PADDING) * 2]):
            raise SoundFontError('Missing 46-point zero padding')
        # Links need not be symmetric: FluidR3 marks L/R samples but links them all to sample 0.
        if kind in LINKED_TYPES and link >= len(sf.shdr) - 1:
            raise SoundFontError('Dangling stereo link')
    if _name(sf.shdr[-1][0]) != 'EOS' or len(sf.smpl) // 2 != sf.shdr[-2][2] + ZERO_PADDING:
        raise SoundFontError('Terminal sample record or smpl size is wrong')
    return sf


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--list', action='store_true', help='list presets as bank:program name')
    parser.add_argument('--keep', nargs='*', default=[], help='bank:program presets to keep')
    parser.add_argument('-o', '--output', type=Path)
    args = parser.parse_args()
    sf = SoundFont(args.source.read_bytes())
    if args.list:
        for bank, program, name, _ in sorted(sf.presets()):
            print(f'{bank}:{program} {name}')
    if args.keep:
        if not args.output:
            parser.error('--keep needs --output')
        wanted = [tuple(int(x) for x in item.split(':')) for item in args.keep]
        args.output.write_bytes(subset(sf, wanted))
        print(f'Wrote {args.output} ({args.output.stat().st_size:,} bytes)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
