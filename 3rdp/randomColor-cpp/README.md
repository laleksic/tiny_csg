randomColor-cpp
---------------

This is a C++ random color generator inspired by and based on David Merfield's 
[randomColor] (https://github.com/davidmerfield/randomColor) (but not an 
exact port). The generator uses the HSB color space and predefined color 
map to produce colors.

The code was compiled with mingw 5.1.0. It requires support of C++11,
particularly initializer lists, mutexes and lambdas.

![example](/example.png)


Differences from the original project
-------------------------------------

- Added upper bound for brightness to support colors like brown.
- Light/dark/bright shades are calculated in a slightly different way and 
  with relative values, so that produced colors always remain within the 
  color map.
- Added "Normal" luminosity option to produce colors which are not too 
  light or dark. This option is set by default.
- Added intermediate color ranges like RedOrange, Cyan, Magenta, etc.
- Added option to specify a set of possible colors, one of which should be
  generated: generate({Red, Orange, Yellow}, ...). The probability of each 
  color depends on its hue range size. There are predefined sets like 
  AnyRed, AnyBlue, etc.
- Added option to generate a color within custom hue range, like 
  generate({100, 150}, ...).
- The color map may contain hue ranges which intersect each other. When 
  the generator needs to find a range containing some hue, it selects the 
  narrowest one.
- The color map is changed.
- Several instances of the color generator can be created, each with its 
  own seed. The seed can be changed at any time.
- A generated color is always returned in 0xRRGGBB format.
- The internal random generator is std::default_random_engine.
- There is no option to generate an array of colors in one call.


Example
-------

```cpp
RandomColor randomColor;

// Random color with normal luminosity
int c1 = randomColor.generate();

// Random light red color
int c2 = randomColor.generate(RandomColor::Red, RandomColor::Light);

// Random dark color with hue within [150; 160]
int c3 = randomColor.generate({150, 160}, RandomColor::Dark);

// Random yellow or orange color with normal luminosity
int c4 = randomColor.generate({RandomColor::Yellow, RandomColor::Orange});
```


Known issues
------------

Colors which are naturally bright (like yellow, pink) or dark (like red, 
blue) can become too bright/dark with corresponding luminosity. The generator 
selects all colors in the same way, not considering this "natural brightness". 
This could be solved by including additional parameter in the color map or 
by making this map more complex.

The generator can produce similar colors in a row. Distinct hue values are 
not always enough to generate distinct colors, but this could be a simple 
solution.

Adjacent hue ranges can produce similar colors. One could try to add unused 
gaps between them, e.g. by defining ranges like [11, 18] and [24, 41], where 
[19, 23] is not used. But these gaps should also be present in the color map, 
otherwise the generator would use RandomHue parameters for them.

The color map can be improved (or configured for a particular purpose).


Credits
-------

Thanks to David Merfield, the author of the original randomColor. 

Also thanks to the authors of http://www.workwithcolor.com, which contains 
a lot of useful information (their color ranges definition helped to create
the color map).

And thanks to the authors of wikipedia articles like that: 
- https://en.wikipedia.org/wiki/HSL_and_HSV
- https://en.wikipedia.org/wiki/Lists_of_colors
