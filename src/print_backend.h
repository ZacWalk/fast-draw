// Explicit application print backend.  libdesktop owns the window/reactor
// boundary; Windows' printer dialog and printer DC are specific to this
// document renderer and therefore stay isolated here.

#pragma once

class View;

bool PrintDocument(View& view);
