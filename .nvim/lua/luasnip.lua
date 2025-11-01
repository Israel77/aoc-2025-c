Snacks.notify("Hello, world!")

local ls = require("luasnip")

ls.add_snippets("all", {
    ls.parser.parse_snippet("/.", "/* $0 */"),
})
