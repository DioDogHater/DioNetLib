# What is this library about?
- Simple networking utilities
- Easy compatibility between Windows and Linux
- Making networking shorter and clearer

# How do I use it?
You add it to the directory of your source C/C++ files and you include it. Please read the information written in the dionetlib.h header file for more information.
Use my library however you want, just don't forget to credit me if your project goes commercial or popular. Otherwise, use my code, edit it, whatever just credit.
Also, when compiling on Windows, you will have to use Mingw (any version) and add the `-lws2_32 -lmswsock -ladvapi32` flags after your source file.
Here is an example: `gcc -Wall main.c -lws2_32 -lmswsock -ladvapi32 -o main.exe`. For linux, no need for anything to be added, just compile using a valid C compiler.
BTW, since most functions included in the library are actually macros, you can indeed remove the semi-columns after using a macro.
Since I've already put a semi-colon at the end of each line of the macros, the compiler could care less if you have one after them.
Also, the macros you see that have no parentheses after them are actually called without them.
For example, the macros `bind_server_socket`, `listen_server_socket`, `close_server_socket`, `connect_client_socket` and `close_client_socket` do not need parentheses after them.
I know it looks horrible in plain C code, but I hate having to switch between macros and functions, so deal with it.