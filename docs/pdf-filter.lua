-- Pandoc filter for the report PDFs.
-- <br> inside table cells becomes a line break; <!-- pagebreak --> starts a new page.
function RawInline(el)
  if el.format == "html" and el.text:match("^<br%s*/?>$") then return pandoc.LineBreak() end
end

function RawBlock(el)
  if el.format == "html" and el.text:match("^<!%-%-%s*pagebreak%s*%-%->$") then
    return pandoc.RawBlock("latex", "\\newpage")
  end
end
