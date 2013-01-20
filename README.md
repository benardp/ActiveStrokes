This is the research prototype implementing the image space line tracking, parameterization and stylization method described in the following publication:

Active Strokes: Coherent Line Stylization for Animated 3D Models
Pierre Bénard; Lu Jingwan; Forrester Cole; Adam Finkelstein; Joëlle Thollot
NPAR 2012 - 10th International Symposium on Non-photorealistic Animation and Rendering.
http://hal.inria.fr/hal-00693453/en

The code is based on dpix: http://gfx.cs.princeton.edu/proj/dpix/

Note: This is research software. As such, it may fail to run, crash, or otherwise not perform as expected. It is not intended for regular use in any kind of production pipeline.

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

Tested on Ubuntu 12.04 with gcc (4.6), Mac OSX 10.8 with gcc (4.2), and Windows 7 with Visual Studio 2008.

Any questions or problems, email pierre.benard@laposte.net
Jan 2013