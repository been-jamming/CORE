" Vim syntax file
" Language: CORE proof assistant
" Maintainer: Benjamin Jones
" Latest Revision: 4 April 2021

if exists("b:current_syntax")
	finish
endif

" Commands
syn keyword CORE_commands return or
syn keyword CORE_commands define axiom prove given choose implies not print debug

syn keyword CORE_commands return
syn keyword CORE_commands or
syn keyword CORE_commands define
syn keyword CORE_commands axiom
syn keyword CORE_commands prove
syn keyword CORE_commands given
syn keyword CORE_commands choose
syn keyword CORE_commands implies
syn keyword CORE_commands not
syn keyword CORE_commands print
syn keyword CORE_commands debug
syn keyword CORE_commands rename
syn keyword CORE_commands relation

" Functions
syn keyword CORE_functions left
syn keyword CORE_functions right
syn keyword CORE_functions and
syn keyword CORE_functions or
syn keyword CORE_functions branch
syn keyword CORE_functions expand
syn keyword CORE_functions swap
syn keyword CORE_functions iff

" Comments
syn match CORE_comment "\/\/.*$"

"Operators
syn match CORE_operator "\V~"
syn match CORE_operator "\V*"
syn match CORE_operator "\V^"
syn match CORE_operator "\V&"
syn match CORE_operator "\V|"
syn match CORE_operator "\V->"
syn match CORE_operator "\V<->"
syn match CORE_operator "\V#"
syn match CORE_operator "\V="

let b:current_syntax = "core"

hi def link CORE_commands Statement
hi def link CORE_functions Function
hi def link CORE_comment Comment
hi def link CORE_operator Operator

