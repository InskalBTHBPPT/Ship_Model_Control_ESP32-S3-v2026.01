"""Regenerate Ship Auto Way Maps Points Tracking.pdf from markdown."""
from pathlib import Path

import markdown
from xhtml2pdf import pisa

BASE = Path(__file__).resolve().parent
MD_PATH = BASE / "Ship Auto Way Maps Points Tracking.md"
PDF_PATH = BASE / "Ship Auto Way Maps Points Tracking.pdf"

text = MD_PATH.read_text(encoding="utf-8")
html_body = markdown.markdown(text, extensions=["tables", "fenced_code"])

html = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<style>
body {{ font-family: Arial, Helvetica, sans-serif; font-size: 10pt; margin: 32px; line-height: 1.4; color: #111827; }}
h1 {{ font-size: 18pt; color: #1f2937; border-bottom: 2px solid #3b82f6; padding-bottom: 6px; }}
h2 {{ font-size: 13pt; color: #374151; margin-top: 18px; page-break-after: avoid; }}
h3 {{ font-size: 11pt; margin-top: 12px; page-break-after: avoid; }}
table {{ border-collapse: collapse; width: 100%; margin: 10px 0; font-size: 9pt; }}
th, td {{ border: 1px solid #d1d5db; padding: 5px 6px; text-align: left; vertical-align: top; }}
th {{ background: #f3f4f6; font-weight: bold; }}
code {{ font-family: Consolas, monospace; font-size: 8pt; background: #f9fafb; }}
pre {{ font-family: Consolas, monospace; font-size: 8pt; background: #f9fafb; border: 1px solid #e5e7eb; padding: 8px; white-space: pre-wrap; }}
ul, ol {{ margin: 6px 0; }}
li {{ margin: 3px 0; }}
hr {{ border: none; border-top: 1px solid #e5e7eb; margin: 16px 0; }}
</style>
</head>
<body>
{html_body}
</body>
</html>"""

with open(PDF_PATH, "wb") as f:
    status = pisa.CreatePDF(html, dest=f, encoding="utf-8")
    if status.err:
        raise SystemExit(f"PDF generation failed, error count: {status.err}")

print(f"Created: {PDF_PATH} ({PDF_PATH.stat().st_size} bytes)")
