"""Convert textparser's generated raw regex literals to C11 string literals."""

import json
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
text = path.read_text(encoding="utf-8")
text = re.sub(
    r'R"regex\((.*?)\)regex"',
    lambda match: json.dumps(match.group(1), ensure_ascii=False),
    text,
    flags=re.DOTALL,
)
path.write_text(text, encoding="utf-8")
