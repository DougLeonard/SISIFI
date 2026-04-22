# Subscribable-Inheritance Shadow Interfaces (SISI) and the SISI Fluent Idiom (SISIFI)  for C++
*A Novel C#-equivalent, simple, highly-native, zero-overhead, polymorphic fluent design for C++, by Douglas S. Leonard.*

I only figured this out after, or while, writing [DLG4::VolumeBuilders](dsleonard-coding.github-io/VolumeBuilders).  The type-erased CRTP method used in VolumeBuilders is nice to avoid boilerplate wrappers, but it's a bit abstract, and saying it wasn't simple to get right is a huge understatement. The common types are actually concrete builders that can be constructed from other builders by linking to their data.  Getting views, cloning, and (limited) polymorphism right is/was very hard.  

## The goal

The goal is to get methods that always return pointers or references to concrete type when called from a concrete pointer, even if calling a base method, so that we can chain calls and retain access to non-virtual concrete methods,  while also allowing full polymorphic access through common interface pointers/references.  This is the standard problem to solve for ideal polymorphic fluent interfaces.  But the real goal is to do it without hiring the entire Microsoft programming team.

## How C# does this  
C# inspired me to look harder at a simple solution. The idea is using a CRTP base method with a separate interface method having interface return type.  It sounds too simple, but of course doesn't work in C++, except actually it does.  But first, C# effectively calls the CRTP method from the concrete object and the interface method from the  interface view. Both methods are definable in the code block of the CRTP base class. If an interface method is explicitly defined, it inherits from (is polymorphically callable by) the interface. Then the class method that happens to have the same name and signature is not actually constrained by the interface or by inheritance.  It's just a method having the same name and signature.  The  class method is reached through calls on the class and the interface method is reached through calls on the interface.  The connection is that the interface method would typically just delegate to the base/concrete method (CRTP base for a fluent setup), completing the polymorphism.  In a fluent setup the class method will return the class type (or derived class usually) and the interface method will cast the return to the interface type.  That is where C++ breaks down.   

## Why C++ supposedly can't  do this
MUCH searching, testing,  and AI querying (which presumably knows common patterns) could not get at a way to overcome the fact that, in C++, templated classes with methods returning the derived types cannot inherit from an interface method with a singular (common) un-templated interface return type.  Covariant return types simply won't work with the class types themselves.  It's not at all clear that anything technically prevents it, but it's not how the language works.   The overwhelmingly common solution found by searching is to use type erasure (of more complex forms than used even in DLG4::VolumeBuilders).  

## The SISIFI (CCFee?) solution (aka the problem that didn't exist)
The solution I finally realized from C# is... **just _don't_ inherit those methods!!  Why try to solve a problem that isn't THE problem?**  We can just write un-inherritted methods with the same name, just like C#, or in C++ lingo, "hide" or "shadow" the interface implementation methods.  

You do exactly what C# does.  The interface implementation and CRTP base class methods should be unlrelated methods. The inheritance tree is  IBuilder <-- IBuilderImpl<ConcreteBuilder> <-- BuilderBase<ConcreteBuilder> <-- ConcreteBuilder.
IBuilder just serves as the common interface and polymorphically calls interface implementations in IBuilderImpl<ConcreteBuilder> that delegate to class methods in BuilderBase<ConcreteBuilder>.  Because the BuilderBase methods do not inherit from IBuilderImpl, but hide them, they can return pointers to ConcreteBuilder when called through ConcreteBuilder, while the IBuilderImpl delegators return pointers to the IBuilder when called through IBuilder.  That's it.  You get the same thing as C# with the only difference being that the syntax, in that interface impelementations live inside a separate IBuilderImpl code block instead of inside the BuilderBase class block. 

**The [code is here](src/SISIFI_example.cc) and outputs as expected.**, demostrating full decayless fluency on the concrete builders and fully collectible/polymorphic  wiring to them or the CRTP base from the interface.

code output:
```
>> This is a vehicle with 2000 cc displacement, 0 wheels,  and a  car body with 2 doors  
You should add some wheels.

  Assembly Line:  

>>> This is a vehicle with 1200 cc displacement, 4 wheels,  and a  car body with 2 doors  
Bikes have 2 wheels, silly!
>>> This is a vehicle with 1200 cc displacement, 2 wheels,  and a  bike body  
```


## The SISI mechanism

In the SISIFI method, the shadowing is done by adding an extra tag parameter to the IBuilder method signature, with a default value, so that it doesn't need to be provided, like void* = nullptr.  IBuilderImpl overrides and matches this signature, but BuilderBase leaves the parameter out, creating a shadow overload instead of an override.  The hiding method should again be marked virutal, certainly in a fluent CRTP base at least.  You can use a macro to add clarity, like 
```cpp
#define SISIFI_TAG std::integral_constant<int,0> = {}
```  
The syntax is a little idiosyncratic, but concise, and it's C++ anyway.   Using std::integral_constant for the type allows general SISI1, SISI2, etc tags for multi-level, subscribable shadowing if needed.  You  re-declare the overload as virtual and tag it with a 
```cpp
#define SISI1 std::integral_constant<int,1> = {}
```   
This provides subscribable multi-channel virtual methods. And as they are marked with clear intent, it goes from being a dangerously unobvious side effect, to a clearly deliberate architectural solution.


## Discussion and performance

You have fully polymorphic IBuilder views. This creates a little boilerplate, just as in C#.  It's less syntactically idiomatic than in C#, but it creates a very straightforward hierarchical solution. If we didn't mind calling IDoWork() from the interface and DoWork() from the class, there wasn't even an issue.  The only issue was getting the same name, and the traditional wisdom I could find has been that you cannot do that with inheritance.  It's hard to find evidence that anyone ever thought "so don't inherit."  

Overhead can be essentially zero.  If all of this except the concrete class is in header code, and if you use private friendship for inheritance, the compiler can see everything up to the BuilderBase vtable which it should essentially transplant into the IBuilder vtable. At worst, if you at least mark the IBuilderImpl methods final, it should be only one extra vtable jump, which can't be worse than alternatives. The delegation will surely optimize away. The downside here is you do have to pay the boilerplate tax for the interface and delegator for every base class method, not just polymorphic ones. This is no worse than in C#, but some other methods can avoid that. The gain is that this approach is extremely straightforward compared to Concept/Model/View type-erasing schemes which can be very complex to implement (like the schemes used in VolumeBuilders).  

## Multi-level inheritance with SISIFI

SISIFI scales well for multi-level inheritance. In [DLG4::VolumeBuilders](dsleonard-coding.github-io/VolumeBuilders), it could include VolumeBuilder and StructureBuilder naturally, and eliminates a lot of conversion ctors, view links, variadic ctors, header loops, type erasure, and pains in the neck.
Expanding to more levels works exactly like in C# with the same options available.   IBase inherits from and adds to the list of method signatures from ISubBase.   Base will inherit from IBaseImpl, with new virtual methods that don't subscribe to SISIFI_TAG inherittance.  Base can either also inherit from SubBase (with virutual inheritance to deal with multiple IBase inherritance routes) or can just reimplement the full IBase method set.  IBaseImpl will delegating accordingly to SubBase or to Base for those methods.  You can use a single template parameter passed all the way from Concrete to ISubBaseImpl (or could add a paramenter or the Base too).  If rewriting VolumeBuilders to use SISIFI, I would probably implement structure methods and volume methods fully separately (as they are now as well) as they cannot share code anyway. Even if I did inherit volumes from structures I'd have to override everything. 


##  Why has this has been hard to see and is it safe?
It has been hard to see because C++ inheritance concepts just traditionally aren't meant to limit inheritance to one level, and it's not the normal way of thinking.  Mostly that it's it, although it's compounded by the fact that hiding is both by syntax and history a side-effect, not a feature, and something usually warned about, not leveraged.  It literally produces a compiler warning, -Woverload-virtual. 

Still,  A danger of hiding is that there is potential for surprises. In this pattern the tag is added to abstract classes at the interface level.  It must be inherited at the interface implementation to satisfy the contract, and the base class must define a method to satisfy the delegation, so that's all pretty safe.  Probably the worst danger is forgetting to mark the next level method as virtual, if you need continued polymorphism. Normally it remains virtual.  Here it's a new method, so it can be virtual or not.  This adds flexibility, but a little risk, none without compiler warnings though.

From my searching, this is NOT a common idiom for fluent classes, and it even took a lot of coercing and even arguing to get AI to generate an example of it, because it had no familiarity with the pattern, but rather knew only of reasons to not expect it could work, or diverted to other patterns that missed the point.  I can't say this pattern doesn't exist or that it's the best, but it's not in wide enough use for (free) AI to recognize or even easily "comprehend" it, and it's exactly equivalent to more modern languages like C#.  

