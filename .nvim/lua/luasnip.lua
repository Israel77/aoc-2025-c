local ls = require("luasnip")

ls.add_snippets("c", {
    ls.parser.parse_snippet("/.", "/* $0 */"),
})


