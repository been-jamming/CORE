" Vim syntax file
" Language: CORE proof assistant
" Maintainer: Benjamin Jones
" Latest Revision: 11 April 2022

if exists("b:current_syntax")
	finish
endif

" Commands
syn keyword CORE_commands return
syn keyword CORE_commands or
syn keyword CORE_commands define
syn keyword CORE_commands axiom
syn keyword CORE_commands prove
syn keyword CORE_commands given
syn keyword CORE_commands choose
syn keyword CORE_commands assume
syn keyword CORE_commands not
syn keyword CORE_commands print
syn keyword CORE_commands object
syn keyword CORE_commands debug
syn keyword CORE_commands rename
syn keyword CORE_commands relation
syn keyword CORE_commands context
syn keyword CORE_commands import
syn keyword CORE_commands include
syn keyword CORE_commands dependent

" Functions
syn keyword CORE_functions left
syn keyword CORE_functions right
syn keyword CORE_functions and
syn keyword CORE_functions or
syn keyword CORE_functions branch
syn keyword CORE_functions expand
syn keyword CORE_functions iff
syn keyword CORE_functions trivial
syn keyword CORE_functions substitute

" Comments
syn match CORE_comment "\/\/.*$"

" Operators
syn match CORE_operator "\V~"
syn match CORE_operator "\V*"
syn match CORE_operator "\V^"
syn match CORE_operator "\V&"
syn match CORE_operator "\V|"
syn match CORE_operator "\V->"
syn match CORE_operator "\V<->"
syn match CORE_operator "\V#"
syn match CORE_operator "\V="

" Strings
syn region CORE_string start=/\v"/ end=/\v"/

let b:current_syntax = "core"

hi def link CORE_commands Statement
hi def link CORE_functions Function
hi def link CORE_comment Comment
hi def link CORE_operator Operator
hi link CORE_string String

