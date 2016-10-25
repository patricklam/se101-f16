#Coding Guidelines
- Be consistent.
- The length of a line should be at most 120 characters.
- Use spaces instead of tabs. Both 2-space and 4-space tabs are acceptable. Energia defaults to using spaces.
- Use braces and new-lines.
    - Ternary operator expressions should be terse.
  - If a scope contains *another* scope, e.g. `if` inside a `for`, the outer scope should use braces!
- Use constant integers, if possible, to store the length of arrays. Otherwise use `#define`.
- Use `switch` for stringing together equality checks for number-like types: `char`, `int`, ...
- Avoid commenting every line of code. Consider reworking your naming and control flow for clarity if you feel the need to explain your code.
- Write descriptive names for variables, structs, and functions.
- Block comment functions that *cannot* be named clearly! See [here](Coding_Guideline.md#functions) for an example.
- Avoid the comma operator. It rarely improves the quality of your code.
  - This applies to variable declarations as well!

## Details on Guidelines
### Spacing
Use 2, default on Energia, or 4 space tabs. Do not use the tab character itself. Here's why:
- The tab was intended to save space. Space, for code, is not a problem anymore.
- The tab was intended to save time. Integrated Development Environments (IDEs) substitute tabs for spaces so time is not an issue anymore. All you have to do is configure your IDE correctly. Energia uses spaces by default.
- Spaces allow for half-tabs. You may want to emphasize code by half-tabbing in/out of the column. This is not possible with the tab unless you add spaces. Once you start using spaces on tabs, you enforce a policy on tab width.

### Commenting and Naming
Avoid commenting every line of code. Use good names and control flow to explain your solution. The only variables that do not need self-explanatory names are iterators, i.e. the variables used in `for`-loops. By the way, Apple is a huge fan of long, descriptive names...

[IMServiceApplicationGroupListAuthorizationSupport](https://developer.apple.com/reference/imserviceplugin/imserviceapplicationgrouplistauthorizationsupport)

Oh yeah. That's real. Never hesitate to write a descriptive name.

*ASIDE &dash; Notice they didn't name it `InstantMessagingServiceApplicationGroupListAuthorizationSupport`!*

#### Variables
Variable names should explain what they store or what they are used for. You might have a boolean named `shouldExit` or an integer named `lengthStrInput`. A hard-and-fast set of rules to follow are:
- If the variable indicates an action or modifies control flow, use verbs.
- If the variable stores data, use nouns.

Here are a few examples from my code:

| File System Constant |  |
| -------------------------:| ----- |
|*Requirement*  | Save the location in EEPROM memory where the files are kept.|
|*Variable Name*| `FileSystemDataBegin` |
|*Link*         | [File_System.c](projects/Project_Storage/File_System.c#L12) |
|*Example Use*  | `FileSystemDataBegin + bitsFoundAt * sizeof(uint32_t)` |

| Button Press Signal |  |
| -------------------------:| ----- |
|*Requirement*  | Indicate a change in the button state, specifically a rising edge. |
|*Variable Name*| `ButtonState.isRising` |
|*Link*         | [Game_UI.ino](projects/Project_RPS/Game_UI.ino#L24) |
|*Example Use*  | `if(gameInputState.buttons[0].isRising)` |

| Player's Chosen Action |  |
| -------------------------:| ----- |
|*Requirement*  | Store the actions of the current round of Rock-Paper-Scissors. |
|*Variable Name*| `GameState.playerActions[MaximumPlayers]` |
|*Link*         | [Game_UI.ino](projects/Project_RPS/Game_UI.ino#L39) |
|*Example Use*  | `rpsDidILose(activeGame.playerActions[i], activeGame.playerActions[j])` |


There is more nuance to naming but these rules suffice.

#### Functions
```
static void draw();
```
Is this a well-named function? No! It invites the question: draw what? It is likely the case that the developer of this function could've described what the function was drawing without yielding a large, unwieldy name. One may find these short names adequate if supplied with context, e.g. class instance, parameter types to dispatch on. This is the case in C if you have an adequately named type in the parameter, such as:
```
static void draw(Circle a);
```

If the function cannot be described in a short name, at the very minimum a block comment should describe it like so:
```
/** Nameserver Registration Method
 *
 *  This method is called by user tasks to register themselves to
 *  a particular name. Currently registration of all names must be performed before any
 *  WhoIs requests are served. This may lead to race conditions if the
 *  initial tasks are not structured correctly. Use with caution.
 *
 *  @param name The Name the calling task should be registered to.
 *  @return Zero on success.
 */
S32 nsRegister( TaskName name );
```
This is a function pulled straight out of my team's CS452 (Real Time) kernel implementation. Notice that the name cannot provide the detailed information necessary to use this function safely and effectively!

Your parameters should follow the variable naming convention.

Here is another example of poor naming, from my own code:
```
bool rpsDidILose(int a, int b)
```
Who lost to who? Good question!

### Control Flow
- Do not use `goto`. There is a time and place for using `goto` and, if you like, you can ask me about it.
- Don't forget about `do-while`! Very useful if you need at least one iteration before checking a condition!
- Use `switch` for stringing together a large number of equality conditions for number-like types: `char`, `int`, ...
  - [Fall-through](http://stackoverflow.com/questions/8146106/does-case-switch-work-like-this) is safe to use if the two cases should execute the same code.

## Other Topics

### The Ternary Operator
The ternary operator (`?`) is powerful. It allows a developer to express a short expression that is dependent on a condition. An excerpt of good use, shown below, is found in my Lab 1 solution.
```
digitalWrite( LED, (code & 1) ? HIGH : LOW );
```
The operation in the condition is simple &dash; not more than a few operators &dash; and the expressions are simple constants. The expression can also contain function calls or arithmetic but they should be concise.

### The Comma Operator
You may be inclined to use the comma operator (`,`). A comma joins and evaluates expressions in sequence in addition to evaluating to the value of the *last* expression. For example,
```
static int value = 0;
int f(int x) {
  return value += x * x;
}

int g(int x) {
  return 2 * x + value;
}

...
int main() {
  /**
   * f(10);
   * int c = g(10);
   **/
  int c = ( f(10), g(10) );
  _Bool shouldContinue = true;

  // while( c == g(10) )
  while( c > 110, shouldContinue ) {
    printf("%d\n", c);
    c--;
  }
}
```
The code above enters an infinite loop! In fact, the compiler throws away `c > 110` since the result is unused. This is certainly not what was intended!

Commas are also used to delimit variable declarations like so:
```
int a,
    b,
    c;
```
Why is this bad? Let us try to make all of these variables pointers to integers.
```
int *a,
    *b,
    *c;
```
Are additional type specifiers/qualifiers for one variable only? Nope! Consider:
```
const int *a,
          *b,
          *c;
```
Now *all* the variables are of type `const int *`. It is worth pointing out that rewriting the type as `int const *` does not help. This has to do with the structure of the C Grammar. Some mistakes can be caught by the compiler but others may pass as warnings and, on a bad day, you may just miss it! It is easy to make mistakes using commas when mentally fatigued. Use them at your peril.
