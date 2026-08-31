#!/usr/bin/env python3
"""Build the candidate-only static DATATBLS_CompileTxt inventory for D2R 3.3."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sqlite3
import sys
from pathlib import Path
from typing import Any

import capstone
from capstone import x86_const
import pefile


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
WORKBENCH_ROOT = REPOSITORY_ROOT / "reverse-engineering" / "d2r-3.2.92777"
WORKBENCH_PATH = WORKBENCH_ROOT / "workbench.json"
CATALOG_PATH = WORKBENCH_ROOT / "datatables-atlas" / "catalog.json"
DEFAULT_OUTPUT_PATH = WORKBENCH_ROOT / "datatables-atlas" / "candidates.json"
COMPILE_TXT_RVA = 0x2FF970
VOLATILE_REGISTERS = {"rax", "rcx", "rdx", "r8", "r9", "r10", "r11"}
HEADER_PATTERN = re.compile(r"^[\x20-\x7E]{1,64}$")

REGISTER_ALIASES = {
    "rax": "rax", "eax": "rax", "ax": "rax", "al": "rax", "ah": "rax",
    "rbx": "rbx", "ebx": "rbx", "bx": "rbx", "bl": "rbx", "bh": "rbx",
    "rcx": "rcx", "ecx": "rcx", "cx": "rcx", "cl": "rcx", "ch": "rcx",
    "rdx": "rdx", "edx": "rdx", "dx": "rdx", "dl": "rdx", "dh": "rdx",
    "rsi": "rsi", "esi": "rsi", "si": "rsi", "sil": "rsi",
    "rdi": "rdi", "edi": "rdi", "di": "rdi", "dil": "rdi",
    "rbp": "rbp", "ebp": "rbp", "bp": "rbp", "bpl": "rbp",
    "rsp": "rsp", "esp": "rsp", "sp": "rsp", "spl": "rsp",
}
for register_index in range(8, 16):
    REGISTER_ALIASES[f"r{register_index}"] = f"r{register_index}"
    REGISTER_ALIASES[f"r{register_index}d"] = f"r{register_index}"
    REGISTER_ALIASES[f"r{register_index}w"] = f"r{register_index}"
    REGISTER_ALIASES[f"r{register_index}b"] = f"r{register_index}"


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest().upper()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def format_rva(value: int) -> str:
    return f"0x{value:X}"


def resolve_cache_path(relative_path: str) -> Path:
    candidate = (WORKBENCH_ROOT / relative_path).resolve()
    cache_root = (WORKBENCH_ROOT / "analysis-cache").resolve()
    if cache_root not in candidate.parents:
        raise RuntimeError(f"workbench artifact escapes analysis-cache: {relative_path}")
    return candidate


def verify_file(path: Path, specification: dict[str, Any], label: str) -> bytes:
    if not path.is_file():
        raise RuntimeError(f"{label} is missing: {path}")
    content = path.read_bytes()
    if len(content) != int(specification["size"]):
        raise RuntimeError(f"{label} size mismatch")
    actual_hash = sha256_bytes(content)
    if actual_hash != specification["sha256"].upper():
        raise RuntimeError(f"{label} SHA-256 mismatch: {actual_hash}")
    return content


def read_rva(pe: pefile.PE, image: bytes, rva: int, size: int) -> bytes:
    try:
        offset = pe.get_offset_from_rva(rva)
    except pefile.PEFormatError as error:
        raise RuntimeError(f"RVA {format_rva(rva)} is not file-backed") from error
    return image[offset : offset + size]


def read_ascii(pe: pefile.PE, image: bytes, rva: int, maximum: int = 96) -> str | None:
    try:
        raw = read_rva(pe, image, rva, maximum)
    except RuntimeError:
        return None
    terminator = raw.find(b"\0")
    if terminator < 1:
        return None
    candidate = raw[:terminator]
    try:
        text = candidate.decode("ascii")
    except UnicodeDecodeError:
        return None
    return text if HEADER_PATTERN.fullmatch(text) else None


def normalize_register(instruction: capstone.CsInsn, register_id: int) -> str | None:
    return REGISTER_ALIASES.get(instruction.reg_name(register_id).lower())


def instruction_evidence(instruction: capstone.CsInsn, image_base: int) -> dict[str, str]:
    return {
        "rva": format_rva(instruction.address - image_base),
        "bytes": bytes(instruction.bytes).hex(" ").upper(),
    }


def canonicalize_instruction_evidence(
    evidence: dict[str, str], canonical_pe: pefile.PE, canonical_image: bytes
) -> dict[str, Any]:
    rva = int(evidence["rva"], 16)
    instruction_bytes = bytes.fromhex(evidence["bytes"])
    canonical_bytes = read_rva(
        canonical_pe, canonical_image, rva, len(instruction_bytes)
    )
    return {
        **evidence,
        "canonicalByteExact": instruction_bytes == canonical_bytes,
    }


def copy_value(value: dict[str, Any] | None) -> dict[str, Any] | None:
    return None if value is None else dict(value)


def value_with_evidence(
    value: dict[str, Any], instruction: capstone.CsInsn, image_base: int
) -> dict[str, Any]:
    result = dict(value)
    result["definition"] = instruction_evidence(instruction, image_base)
    return result


def operand_value(
    instruction: capstone.CsInsn,
    operand: Any,
    registers: dict[str, dict[str, Any]],
    image_base: int,
    for_lea: bool = False,
) -> dict[str, Any]:
    if operand.type == x86_const.X86_OP_IMM:
        return value_with_evidence(
            {"kind": "constant", "value": int(operand.imm) & 0xFFFFFFFFFFFFFFFF},
            instruction,
            image_base,
        )
    if operand.type == x86_const.X86_OP_REG:
        register = normalize_register(instruction, operand.reg)
        tracked = copy_value(registers.get(register or ""))
        if tracked is not None:
            return tracked
        return value_with_evidence(
            {"kind": "register", "register": register or instruction.reg_name(operand.reg)},
            instruction,
            image_base,
        )
    if operand.type == x86_const.X86_OP_MEM:
        base = normalize_register(instruction, operand.mem.base) if operand.mem.base else None
        index = normalize_register(instruction, operand.mem.index) if operand.mem.index else None
        displacement = int(operand.mem.disp)
        if operand.mem.base == x86_const.X86_REG_RIP:
            target_rva = instruction.address + instruction.size + displacement - image_base
            kind = "address" if for_lea else "global-load"
            return value_with_evidence(
                {"kind": kind, "rva": target_rva}, instruction, image_base
            )
        if base in {"rbp", "rsp"} and index is None:
            kind = "local" if for_lea else "local-load"
            return value_with_evidence(
                {"kind": kind, "base": base, "displacement": displacement},
                instruction,
                image_base,
            )
        return value_with_evidence(
            {
                "kind": "memory",
                "base": base,
                "index": index,
                "scale": int(operand.mem.scale),
                "displacement": displacement,
            },
            instruction,
            image_base,
        )
    return value_with_evidence({"kind": "unknown"}, instruction, image_base)


def process_until_call(
    instructions: list[capstone.CsInsn], call_rva: int, image_base: int
) -> tuple[dict[str, dict[str, Any]], dict[tuple[str, int], dict[str, Any]], list[dict[str, Any]]]:
    registers: dict[str, dict[str, Any]] = {}
    stack: dict[tuple[str, int], dict[str, Any]] = {}
    local_writes: list[dict[str, Any]] = []

    for instruction in instructions:
        instruction_rva = instruction.address - image_base
        if instruction_rva == call_rva:
            return registers, stack, local_writes
        if instruction.id == 0:
            continue

        handled_registers: set[str] = set()
        operands = instruction.operands
        mnemonic = instruction.mnemonic.lower()

        if (
            mnemonic in {"mov", "movabs", "movzx", "movsx", "movsxd", "lea"}
            and len(operands) >= 2
            and operands[0].type == x86_const.X86_OP_REG
        ):
            destination = normalize_register(instruction, operands[0].reg)
            if destination:
                registers[destination] = operand_value(
                    instruction,
                    operands[1],
                    registers,
                    image_base,
                    for_lea=mnemonic == "lea",
                )
                handled_registers.add(destination)
        elif (
            mnemonic == "xor"
            and len(operands) == 2
            and operands[0].type == x86_const.X86_OP_REG
            and operands[1].type == x86_const.X86_OP_REG
        ):
            left = normalize_register(instruction, operands[0].reg)
            right = normalize_register(instruction, operands[1].reg)
            if left and left == right:
                registers[left] = value_with_evidence(
                    {"kind": "constant", "value": 0}, instruction, image_base
                )
                handled_registers.add(left)
        elif (
            mnemonic in {"add", "sub"}
            and len(operands) == 2
            and operands[0].type == x86_const.X86_OP_REG
            and operands[1].type == x86_const.X86_OP_IMM
        ):
            destination = normalize_register(instruction, operands[0].reg)
            current = registers.get(destination or "")
            if destination and current and current.get("kind") == "constant":
                delta = int(operands[1].imm)
                if mnemonic == "sub":
                    delta = -delta
                registers[destination] = value_with_evidence(
                    {"kind": "constant", "value": (int(current["value"]) + delta) & 0xFFFFFFFFFFFFFFFF},
                    instruction,
                    image_base,
                )
                handled_registers.add(destination)

        if (
            mnemonic in {"mov", "movabs"}
            and len(operands) >= 2
            and operands[0].type == x86_const.X86_OP_MEM
        ):
            memory = operands[0].mem
            base = normalize_register(instruction, memory.base) if memory.base else None
            index = normalize_register(instruction, memory.index) if memory.index else None
            if base in {"rbp", "rsp"} and index is None:
                value = operand_value(instruction, operands[1], registers, image_base)
                key = (base, int(memory.disp))
                stack[key] = value
                local_writes.append(
                    {
                        "base": base,
                        "displacement": int(memory.disp),
                        "size": int(operands[0].size),
                        "value": value,
                        "store": instruction_evidence(instruction, image_base),
                    }
                )

        try:
            _, written_register_ids = instruction.regs_access()
        except capstone.CsError:
            written_register_ids = []
        for register_id in written_register_ids:
            register = normalize_register(instruction, register_id)
            if register and register not in handled_registers:
                registers.pop(register, None)

        if instruction.group(capstone.CS_GRP_CALL):
            for register in VOLATILE_REGISTERS:
                registers.pop(register, None)

    raise RuntimeError(f"callsite {format_rva(call_rva)} was not disassembled")


def expression(value: dict[str, Any] | None) -> str | None:
    if value is None:
        return None
    kind = value.get("kind")
    if kind == "constant":
        return format_rva(int(value["value"]))
    if kind in {"address", "global-load"}:
        prefix = "*" if kind == "global-load" else ""
        return f"{prefix}{format_rva(int(value['rva']))}"
    if kind in {"local", "local-load"}:
        displacement = int(value["displacement"])
        sign = "+" if displacement >= 0 else "-"
        prefix = "*" if kind == "local-load" else ""
        return f"{prefix}{value['base']}{sign}{format_rva(abs(displacement))}"
    if kind == "register":
        return str(value["register"])
    return kind


def public_value(value: dict[str, Any] | None) -> dict[str, Any]:
    if value is None:
        return {"status": "unresolved"}
    result = {"status": "candidate", "expression": expression(value)}
    if "definition" in value:
        result["definition"] = value["definition"]
    return result


def latest_local_values(local_writes: list[dict[str, Any]]) -> dict[tuple[str, int], dict[str, Any]]:
    result: dict[tuple[str, int], dict[str, Any]] = {}
    for write in local_writes:
        result[(str(write["base"]), int(write["displacement"]))] = write
    return result


def descriptor_clusters(
    local_writes: list[dict[str, Any]],
    stride: int | None,
    pe: pefile.PE,
    image: bytes,
    canonical_pe: pefile.PE,
    canonical_image: bytes,
) -> list[dict[str, Any]]:
    latest = latest_local_values(local_writes)
    entries: list[dict[str, Any]] = []
    for (base, displacement), name_write in latest.items():
        name_value = name_write["value"]
        if name_value.get("kind") != "address":
            continue
        name_rva = int(name_value["rva"])
        name = read_ascii(pe, image, name_rva)
        type_write = latest.get((base, displacement + 8))
        offset_write = latest.get((base, displacement + 0x10))
        pad_write = latest.get((base, displacement + 0x18))
        if not type_write or not offset_write or not pad_write:
            continue
        if any(
            int(write["size"]) != 8
            for write in (name_write, type_write, offset_write, pad_write)
        ):
            continue
        type_value = type_write["value"]
        offset_value = offset_write["value"]
        if type_value.get("kind") != "constant" or offset_value.get("kind") != "constant":
            continue
        combined_type = int(type_value["value"])
        type_code = combined_type & 0xFFFFFFFF
        count = (combined_type >> 32) & 0xFFFFFFFF
        record_offset = int(offset_value["value"])
        if not (0 < type_code <= 0x40 and count <= 0x1000):
            continue
        if stride is not None and not (0 <= record_offset < stride):
            continue
        stores = {
            "nameStore": canonicalize_instruction_evidence(
                name_write["store"], canonical_pe, canonical_image
            ),
            "typeStore": canonicalize_instruction_evidence(
                type_write["store"], canonical_pe, canonical_image
            ),
            "offsetStore": canonicalize_instruction_evidence(
                offset_write["store"], canonical_pe, canonical_image
            ),
            "linkerOrPadStore": canonicalize_instruction_evidence(
                pad_write["store"], canonical_pe, canonical_image
            ),
        }
        if not all(evidence["canonicalByteExact"] for evidence in stores.values()):
            continue
        entry = {
            "status": "candidate",
            "name": name,
            "nameRva": format_rva(name_rva),
            "nameReadStatus": (
                "resolved-ascii" if name is not None else "unavailable-in-governed-image"
            ),
            "typeCode": type_code,
            "count": count,
            "recordOffset": format_rva(record_offset),
            "localEntry": f"{base}{'+' if displacement >= 0 else '-'}{format_rva(abs(displacement))}",
            "evidence": stores,
        }
        entry["linkerOrPad"] = public_value(pad_write["value"])
        entries.append({"base": base, "displacement": displacement, "public": entry})

    entries.sort(key=lambda item: (item["base"], item["displacement"]))
    groups: list[list[dict[str, Any]]] = []
    for item in entries:
        if (
            not groups
            or groups[-1][-1]["base"] != item["base"]
            or item["displacement"] - groups[-1][-1]["displacement"] != 0x20
        ):
            groups.append([item])
        else:
            groups[-1].append(item)
    return [
        {
            "status": "candidate",
            "association": "heuristic-local-32-byte-sequence",
            "confidence": "medium" if len(group) >= 2 else "low",
            "entries": [item["public"] for item in group],
        }
        for group in groups
    ]


def build_inventory() -> dict[str, Any]:
    workbench = load_json(WORKBENCH_PATH)
    catalog_bytes = CATALOG_PATH.read_bytes()
    catalog = json.loads(catalog_bytes.decode("utf-8"))
    image_base = int(workbench["target"]["imageBase"], 16)

    canonical_path = resolve_cache_path(workbench["canonicalImage"]["relativePath"])
    analysis_path = resolve_cache_path(workbench["analysisImage"]["relativePath"])
    index_path = resolve_cache_path(workbench["index"]["relativePath"])
    canonical_image = verify_file(canonical_path, workbench["canonicalImage"], "canonical image")
    analysis_image = verify_file(analysis_path, workbench["analysisImage"], "analysis image")
    canonical_pe = pefile.PE(data=canonical_image, fast_load=False)
    analysis_pe = pefile.PE(data=analysis_image, fast_load=False)

    connection = sqlite3.connect(index_path)
    connection.row_factory = sqlite3.Row
    try:
        if connection.execute("PRAGMA quick_check").fetchone()[0] != "ok":
            raise RuntimeError("workbench index quick_check failed")
        metadata = {
            str(row[0]): str(row[1])
            for row in connection.execute("SELECT key, value FROM metadata")
        }
        if metadata.get("imageSha256") != workbench["analysisImage"]["sha256"].upper():
            raise RuntimeError("workbench index image hash mismatch")
        rows = connection.execute(
            """
            SELECT refs.source_rva, refs.function_rva, functions.end_rva
            FROM refs
            JOIN functions ON functions.start_rva = refs.function_rva
            WHERE refs.target_rva = ? AND refs.kind = 'call'
            ORDER BY refs.source_rva
            """,
            (COMPILE_TXT_RVA,),
        ).fetchall()
    finally:
        connection.close()

    disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    disassembler.detail = True
    disassembler.skipdata = True
    candidates: list[dict[str, Any]] = []
    for row in rows:
        call_rva = int(row["source_rva"])
        function_start = int(row["function_rva"])
        function_end = int(row["end_rva"])
        function_bytes = read_rva(
            analysis_pe, analysis_image, function_start, function_end - function_start
        )
        instructions = list(disassembler.disasm(function_bytes, image_base + function_start))
        registers, stack, local_writes = process_until_call(
            instructions, call_rva, image_base
        )
        call_bytes = read_rva(analysis_pe, analysis_image, call_rva, 5)
        canonical_call_bytes = read_rva(canonical_pe, canonical_image, call_rva, 5)
        if len(call_bytes) != 5 or call_bytes[0] != 0xE8:
            raise RuntimeError(f"non-rel32 call indexed at {format_rva(call_rva)}")
        relative_target = int.from_bytes(call_bytes[1:], "little", signed=True)
        resolved_target = call_rva + 5 + relative_target
        if resolved_target != COMPILE_TXT_RVA:
            raise RuntimeError(f"call target drift at {format_rva(call_rva)}")

        source_arguments = []
        for register in ("rdx", "r8", "r9"):
            value = registers.get(register)
            if value and value.get("kind") == "address":
                text = read_ascii(analysis_pe, analysis_image, int(value["rva"]))
                source_arguments.append(
                    {
                        "register": register,
                        "value": text,
                        "rva": format_rva(int(value["rva"])),
                        "readStatus": (
                            "resolved-ascii"
                            if text is not None
                            else "unavailable-in-governed-image"
                        ),
                        "definition": (
                            canonicalize_instruction_evidence(
                                value["definition"], canonical_pe, canonical_image
                            )
                            if value.get("definition")
                            else None
                        ),
                    }
                )

        unique_source_names = sorted(
            {entry["value"] for entry in source_arguments if entry["value"] is not None}
        )
        source_name = unique_source_names[0] if len(unique_source_names) == 1 else None
        unique_source_rvas = sorted({entry["rva"] for entry in source_arguments})
        source_rva = unique_source_rvas[0] if len(unique_source_rvas) == 1 else None
        stride_value = stack.get(("rsp", 0x28))
        literal_stride = (
            int(stride_value["value"])
            if stride_value and stride_value.get("kind") == "constant" and 0 < int(stride_value["value"]) <= 0x100000
            else None
        )
        window_start = max(function_start, call_rva - 32)
        window_size = call_rva + 5 - window_start
        analysis_window = read_rva(analysis_pe, analysis_image, window_start, window_size)
        canonical_window = read_rva(canonical_pe, canonical_image, window_start, window_size)
        clusters = descriptor_clusters(
            local_writes,
            literal_stride,
            analysis_pe,
            analysis_image,
            canonical_pe,
            canonical_image,
        )
        candidate = {
            "id": f"compiletxt-{call_rva:08x}",
            "status": "candidate",
            "callsiteRva": format_rva(call_rva),
            "function": {
                "startRva": format_rva(function_start),
                "endRva": format_rva(function_end),
            },
            "call": {
                "targetRva": format_rva(COMPILE_TXT_RVA),
                "bytes": call_bytes.hex(" ").upper(),
                "canonicalByteExact": call_bytes == canonical_call_bytes,
                "windowStartRva": format_rva(window_start),
                "windowSha256": sha256_bytes(analysis_window),
                "canonicalWindowByteExact": analysis_window == canonical_window,
            },
            "sourceArguments": source_arguments,
            "sourceNameCandidate": source_name,
            "sourceRvaCandidate": source_rva,
            "contextArgument": public_value(registers.get("rcx")),
            "fieldDescriptorArgument": public_value(stack.get(("rsp", 0x20))),
            "recordStride": (
                {
                    "status": "candidate",
                    "value": format_rva(literal_stride),
                    "definition": stride_value.get("definition") if stride_value else None,
                }
                if literal_stride is not None
                else {
                    "status": "unresolved",
                    "expression": expression(stride_value),
                    "definition": stride_value.get("definition") if stride_value else None,
                }
            ),
            "outputArgument": public_value(stack.get(("rsp", 0x30))),
            "descriptorClusters": clusters,
            "limits": [
                "The callsite and literal arguments do not independently prove DataTables records/count ownership.",
                "Descriptor clusters are local 32-byte-shape heuristics and are not promoted fields.",
                "Post-compilation consumers and lifecycle remain outside this extraction.",
            ],
        }
        candidates.append(candidate)

    descriptor_entry_keys = {
        tuple(
            evidence["rva"]
            for evidence in entry["evidence"].values()
        )
        for candidate in candidates
        for cluster in candidate["descriptorClusters"]
        for entry in cluster["entries"]
    }
    summary = {
        "directCallsites": len(candidates),
        "containingFunctions": len({entry["function"]["startRva"] for entry in candidates}),
        "canonicalByteExactCallsites": sum(
            1 for entry in candidates if entry["call"]["canonicalByteExact"]
        ),
        "canonicalByteExactWindows": sum(
            1 for entry in candidates if entry["call"]["canonicalWindowByteExact"]
        ),
        "literalStridesRecovered": sum(
            1 for entry in candidates if entry["recordStride"]["status"] == "candidate"
        ),
        "unanimousSourceNamesRecovered": sum(
            1 for entry in candidates if entry["sourceNameCandidate"] is not None
        ),
        "unanimousSourceRvasRecovered": sum(
            1 for entry in candidates if entry["sourceRvaCandidate"] is not None
        ),
        "callsWithDescriptorClusters": sum(
            1 for entry in candidates if entry["descriptorClusters"]
        ),
        "descriptorClusters": sum(
            len(entry["descriptorClusters"]) for entry in candidates
        ),
        "descriptorEntries": sum(
            len(cluster["entries"])
            for entry in candidates
            for cluster in entry["descriptorClusters"]
        ),
        "uniqueDescriptorEntries": len(descriptor_entry_keys),
    }
    return {
        "schemaVersion": 1,
        "inventoryId": "ruffneckk-d2r33-datatables-compiletxt-candidates-v1",
        "status": "candidate-only",
        "generatedBy": "scripts/reverse-engineering/d2r33-datatables-extract.py",
        "targetRuntime": catalog["targetRuntime"],
        "corpus": {
            "workbenchPath": "reverse-engineering/d2r-3.2.92777",
            "provenanceBuild": workbench["target"]["build"],
            "coveredBuilds": catalog["corpus"]["coveredBuilds"],
            "canonicalImageSha256": workbench["canonicalImage"]["sha256"],
            "analysisImageSha256": workbench["analysisImage"]["sha256"],
            "indexImageSha256": metadata["imageSha256"],
            "catalogSha256": sha256_bytes(catalog_bytes),
        },
        "compiler": {
            "name": "DATATBLS_CompileTxt",
            "rva": format_rva(COMPILE_TXT_RVA),
            "discovery": "direct rel32 calls from the verified persistent index",
        },
        "policy": {
            "promotion": "none",
            "runtimeHooks": False,
            "requiresIndependentProof": [
                "table identity",
                "records slot",
                "count slot",
                "record stride",
                "consumers",
                "post-processing and lifecycle",
            ],
        },
        "summary": summary,
        "candidates": candidates,
    }


def serialize_inventory(inventory: dict[str, Any]) -> str:
    return json.dumps(inventory, indent=2, ensure_ascii=False) + "\n"


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT_PATH)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        output_path = arguments.output.resolve()
        if output_path != REPOSITORY_ROOT and REPOSITORY_ROOT not in output_path.parents:
            raise RuntimeError(f"output path escapes repository: {output_path}")
        content = serialize_inventory(build_inventory())
        if arguments.check:
            if not output_path.is_file():
                raise RuntimeError(f"candidate inventory is missing: {output_path}")
            if output_path.read_text(encoding="utf-8") != content:
                raise RuntimeError(f"candidate inventory is stale: {output_path}")
            inventory = json.loads(content)
            print(
                "VALID candidate inventory: "
                f"calls={inventory['summary']['directCallsites']} "
                f"strides={inventory['summary']['literalStridesRecovered']} "
                f"descriptorEntries={inventory['summary']['descriptorEntries']}"
            )
            return 0
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(content, encoding="utf-8", newline="\n")
        inventory = json.loads(content)
        print(
            "GENERATED candidate inventory: "
            f"calls={inventory['summary']['directCallsites']} "
            f"strides={inventory['summary']['literalStridesRecovered']} "
            f"descriptorEntries={inventory['summary']['descriptorEntries']}"
        )
        return 0
    except (RuntimeError, OSError, ValueError, sqlite3.Error, pefile.PEFormatError) as error:
        print(f"DataTables candidate extraction failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
