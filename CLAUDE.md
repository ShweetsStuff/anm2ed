Code style:
- Use snake_case for functions.
- Use SCREAMING_SNAKE_CASE for constants/constexprs.
- Use camelCase for variables
- Use snake_case for file names and folders.
- Don't use magic numbers, except for the very common stuff like -1, 0, 1, 2. Constants should have a symbol attached to them.
- Newlines for { and }, except if the clause contained inside is one line, then exclude them.
- Booleans (both functions returning them and variables) should always be prepended with "is", even if it grammatically wouldn't make sense.
- Avoid use of anonymous namespaces. For namepsaces, try to have them match files; don't create superfluous namespaces within documents.
-Functions should use snake case and should typically be written like "[noun]_[verb]". Try and use a limited amount of verbs to describe functionality; "save", "load", etc.

If user asks for "how to", or "why", that's an indicator to generate no code and simply answer their prompt.

The most important thing is to do the MOST with the LEAST CODE possible. Always strive to cut down code whenever possible, and to reduce complexity, reuse functions and behaviors, etc.

Relentlessly pause execution to ask the user questions about project architecture, and to guide them and help assist them understand the software being produced.