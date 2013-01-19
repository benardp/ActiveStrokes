This is the research prototype implementing the image space line tracking, parameterization and stylization method described in the following publication:

Active Strokes: Coherent Line Stylization for Animated 3D Models
Pierre Bénard; Lu Jingwan; Forrester Cole; Adam Finkelstein; Joëlle Thollot
NPAR 2012 - 10th International Symposium on Non-photorealistic Animation and Rendering.
http://hal.inria.fr/hal-00693453/en

The library is distributed under the GNU General Public License (GPL), version 3. Please see the COPYING file.

HOW TO BUILD:

You need Qt 4.6 or higher installed. 

On Mac or Linux:

qmake -r ActiveStrokes.pro;
make

On Windows:

Open the Qt command prompt, then

qmake -r ActiveStrokes.pro

Then open ActiveStrokes.sln from Visual Studio and build normally.

Tested on gcc + Ubuntu 10, gcc + Mac OSX 10.8, and Visual Studio 2008 + Windows 7.