# present

A tool to create powerful yet simple presentations from plain-text source files.

---

present is similar to latex+beamer, but the syntax is much smaller than latex, 
thus the compilation time is lesser than latex+beamer. present does not believe
in providing options to the presenter, as that allows them to mess up the 
presentation. Keeping the slides clean and simple is the best way to make good
slides, where the focus should be on the skills of the presenter and his oratory
skills, not using fancy graphics.

For presentations you need two things per slide:

- text (2-3 sentences max)
- image
- title

anything else is just a distraction to the audience. It may look good
aesthetically, but does not help the audience grasp the content any better.

Things that dont belong on a presentation:

- animations
- videos
- multiple images/vectors per slide
- long texts

If presenter requires need of using these, they most probably want a screen-sharing
software instead of a presentation software, as the work of the presentation 
is to aid the presenter.

Things which may be required sometimes:

- lists (like this)
- links

 these are incorporated inside the content rendering system using special syntax

## Syntax

- A banner page (big title only) is marked by a '$' at the begining of a line. Any other component of that slide is ignored
- A title is marked by a '#' symbol at begining of a line
- An image is marked by a '!' symbol at the begining of the line
- A link is marked by a '@' symbol at the begining of the line
- A list is marked by a '-' symbol at the begining of the line
- Content is marked by starting the line without special symbols

New slides are delimited by an empty line. For each slide, the component should
appear in the following order: title > image > content .

If any one the component is absent, it can be omitted.
Each component should start in a new line. Only the content component can
span multiple lines (and the output preserves those line-breaks). Content is 
wrapped if too big to fit in one line.

Lists and Links are part of content itself, and thus are only present at the end.

## Example
```
$Presentation on Seasons

#What are seasons?
They are abstractions of long time-frame weather commonalities which repeat each
year for a particular geographic place on earth.

#How many do we have?
four or five depending on the part of the world you ask the question in.

#They are:
-Summer
-Winter
-Autumn
-Spring
-Monsoon

$How they look?

#Summer
!summer.png

#Winter
!winter.png

#Fall
!autumn.png

#Spring
!spring.png

#Monsoon
!monsoon.png
Monsoon is mostly seen in equitorial areas.

!worldmap.png
An infographic of which country has which seasons

!seasons.png

questions?

thanks!
reach me out on:
@sayn.work sayn.work
@blog.sayn.work My Blog
```

To run the program on an example file go to the example/ folder and execute the program on the given source.pre file
```
present source.pre
```
and an output file of `source.pre.pdf` will be generated. Open it through any pdf viewer and the slides will be visible.

For the above snippet of code to work `present` has to be compiled and the binary has to be in $PATH. otherwise you can refer to it using its path.

```
../bin/present source.pre
```

present expects the presence of a `font.ttf` file which is to be used for the presentation. In the example folder a symlink to the poppins-regular is provided. feel free to change it to any font you like
