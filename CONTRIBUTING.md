# WL4Editor's contributor's guide
Basically this is what you need to know before contributing to this repository.

## TL;DR
- Format code with the clang-format configuration that we provided.
- Make sure the code compiles or mark as [WIP] when making a Pull request (We have a CI and people with Qt Creator so you can't lie to us).
- Avoid using new/delete/malloc/free/realloc, prefer smart pointers for memory management.
- Avoid using C-Style Anything.
- If you have to use C Libraries, use the C++ wrappers.
