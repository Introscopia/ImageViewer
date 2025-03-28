"ImageViewer" is an ultra-minimalistic image viewing program 
for Windows originally built with C and SDL2 by Introscopia in 2020. 
This latest (2025-03-23) update brings this little program to SDL3 and to 64 bits!
It supports webp again, animations, and now sports multi-image capabilities:
Just drag-and-drop image files onto the window!

Uses OK_LIB: https://github.com/brackeen/ok-lib
which is under the MIT license.

https://introscopia.github.io/
introscopia@protonmail.com

> [🡆/🡄 Arrow keys] Navigate to next/previous images in the same folder.
> [Mouse side buttons (mouse X1 and X2)] Also navigate to next/previous images in the same folder.

> [Click&Drag] Pan around.
> [Drag&Drop] image files onto the window to view them together!
> [MIDDLE Click&Drag] Pan around relative to initial mouse press location.
> [🡅/🡇 Arrow keys] pan up and down.

> [ H,J,K,L ] Pan around vim-style. (panning is reversed. You move the "camera", not the image)
> [I, O] Zoom in, out

> NUMPAD:
	> [ 8, 2, 4, 6 ] Pan around. 
	> [ 7, 9 ] rotate the image in increments of 90 degrees.
	> [ 1, 3 ] next and previous image in the folder.
	> [ 5, 0, +, - ] zoom in and out.
	> [ / ] flip horizontally.
	> [ * ] flip vertically.

> [Mouse Wheel] Zoom in/out.
> [CTRL + Click&Drag] select area to zoom into.
> [1...9] to set zoom.
> [spacebar] Fit image to window.

> [A] to cycle through scale quality options:
	NEAREST,
	LINEAR,
	PIXELART (coming soon!)
> [C] to cycle through background colors.

> [F11] toggle fullscreen.
> [F5] reload the file, refresh the list of files in the folder.
> [F6] rebuild the list of files including images in subfolders down to 9999 levels deep.
> [S] shuffle the file list

> [SHIFT + DELETE] permanently delete image (skips recycle bin!!!)

Enjoy!