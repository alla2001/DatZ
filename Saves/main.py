import re
import sys
from pathlib import Path

def parse_blocks(text):
    """
    Parses top-level blocks with their IDs.
    Returns a dict {id: full_block_text}.
    """
    blocks = {}
    stack = []
    current = []
    current_id = None

    for line in text.splitlines():
        # Start of a block
        start_match = re.match(r'^\s*([A-Za-z0-9_]+)\s*"?(?:\{([A-F0-9]+)\})?"?', line)
        if "{" in line:
            stack.append("{")
            if current_id is None:
                current_id = re.search(r'\{([A-F0-9]+)\}', line)
                if current_id:
                    current_id = current_id.group(1)
            current.append(line)

        # Inside block
        elif stack:
            current.append(line)

        # End of a block
        if "}" in line and stack:
            stack.pop()
            if not stack:  # block closed
                block_text = "\n".join(current)
                if current_id:
                    blocks[current_id] = block_text
                current = []
                current_id = None

    return blocks


def merge_layers(file_a, file_b, out_file):
    text_a = Path(file_a).read_text(encoding="utf-8")
    text_b = Path(file_b).read_text(encoding="utf-8")

    blocks_a = parse_blocks(text_a)
    blocks_b = parse_blocks(text_b)

    merged_blocks = blocks_a.copy()

    for bid, block in blocks_b.items():
        if bid not in merged_blocks:
            merged_blocks[bid] = block  # only add new ones

    # Build output text
    merged_text = "\n\n".join(merged_blocks.values())

    Path(out_file).write_text(merged_text, encoding="utf-8")
    print(f"Merged file written to {out_file}")


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python merge_layers.py <file_a.layer> <file_b.layer> <out.layer>")
        sys.exit(1)

    merge_layers(sys.argv[1], sys.argv[2], sys.argv[3])
