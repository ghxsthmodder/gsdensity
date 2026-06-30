# GSDENSITY - CLI Tool encoding and decoding gdm and grle files.
- Note: This version is focused on FS20.

gsdensity
-v, --version
-h, --help

gsdensity encoder
-i, --i3d # optional i3d map
-l, --layers # reuses layers from a gdm/grle file and adds layers to the new gdm/grle file.
-n, --num-channels # Automatic if specified --i3d, retrieves directly from the map's i3d file.
-o, --output (gdm, grle)

gsdensity decoder
-f, --file (*.grle, *.gdm)
-o, --output-dir

gsdensity verify
-f, --file (*.grle, *.gdm)
-n, --num-channels