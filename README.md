# InFact: An easy way to construct C++ objects dynamically

InFact provides an interpreter and factory for the construction of
instances of C++ classes. The syntax is very much like that of C++
itself, and objects may be created that wrap other objects, just like
in C++. It supports all the common primitive types (`bool`, `int`, `double`,
`string`), as well as vectors of primitives or of Factory-constructible
objects. Here's what it looks like:

```
// Construct a cow with a required argument, its name.
Cow c1 = Cow(name("Bessie"));

// Construct a second cow with a different name, and an optional age.
// Also, specifying a type is optional, since InFact does type inference.
c2 = Cow(name("Lani Moo"), age(2));

// Construct a human pet owner with the two cows as pets.
PetOwner p = HumanPetOwner(pets({c1, c2}));
```

Feeding the above to InFact’s
[Interpreter](http://google.github.io/infact/classinfact_1_1_interpreter.html)
class will construct three new objects at run-time (the referents of
`c1`, `c2` and `p`) _without any new C++ code_.

While being very powerful, the library is also quite lightweight: it
is a simple recursive-descent parser with a single token of
look-ahead, with the entire library implemented with fewer than 1000
C++ statements. The inspiration was the
[BeanShell library](http://www.beanshell.org/) for Java, which
provides a very convenient mechanism for the configuration of any Java
project. Similarly, InFact can be used as an effective configuration
mechanism for C++ projects. See
[the ReFr project](https://github.com/google/refr) for an example,
especially
[the default configuration file](https://github.com/google/refr/blob/master/src/reranker/config/default.infact).

The interpreted InFact language supports _type inference_ and _named
parameters_. Compile-time equivalents of these two properties are
available with the Boost Parameter library.

For documentation, please visit
[our documentation site](http://google.github.io/infact/index.html).
