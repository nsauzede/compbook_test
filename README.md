
# Experiments following the Compiler Book
My notes following the excellent [Compiler Book](https://www.sigbus.info/compilerbook).

Each stepXX subdir represents my own interpretation of the book steps cues.
First steps lead to almost identical produced code compared to the cues.
But from steps to steps, the book gives less and less C and asm code cues, which makes the process even more challenging and interesting IMHO.. :-)

The ultimate goal is to proceed through the end of the book, and obtain a self-hosted C compiler ! \o/

Have fun following the book yourself ! I recommend trying it !!

# How to test all the steps

```shell
find . -name "ste*" -type d -exec make  clean all check test -C '{}' \;
```

# Roadmap
## Driver phases
- [ ] preprocessor (.c)
- [x] tokenizer (Token *)
- [x] parser (Obj *)
- [x] code generator (.s)
- [ ] assembler (.o)
- [ ] linker (.elf)
## Compiler features
- [x] compile integer into executable that exits with it
- [x] +,-,*,/,() operators
- [x] if/for/while
- [x] &,* operators, int type and local variable
- [x] functions call
- [x] array type, [] operator
- [x] global var, char type
- [x] string literal
- [x] compile from file
- [x] GNU statement expression
- [x] C tests, Quine support
- [x] octal/hex in strings
- [x] block scope (local vars shadowing)
- [x] .file/.loc
- [x] comma operator
- [x] struct
- [x] -> operator
- [x] union
- [x] short/long, function decl
- [x] void
- [x] typedef, type cast
- [x] bool, enum
- [x] static funcs, loop variables
- [x] assignment ops
- [x] hex/oct/bin literals
- [x] more unary/binary ops
- [x] incomplete types / array to pointer decay
- [ ] goto/labels
- [ ] break/continue/switch
- [ ] do while
- [ ] variadic function
- [ ] signed/unsigned
- [ ] ...
- [ ] self-host ?

# Old notes
*Note: initially I planned to code it in vlang (which would have made it NOT self-hosted ofc), but the clarity of the actual C code samples of the book, made me change my mind :-)*

Once reached, subsequent goals could be : (in no specific order/priority/realism)
- [ ] rewrite the compiler in another language
- [ ] output different assembly than x64  (eg: 16-bit real-mode x86)
- [ ] transpile to another language (eg: vlang, Nelua, etc..)
- [ ] rewrite the compiler to compile another language/grammar than C (eg: V, or brand-new "toy" one..)
- [ ] make this other grammar compiler self-hosted, too
