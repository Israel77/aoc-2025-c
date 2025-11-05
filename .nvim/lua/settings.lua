local ls = require("luasnip")

ls.add_snippets("c", {
    ls.parser.parse_snippet("/.", "/* $0 */"),
})

vim.opt_local.makeprg = './nob'

