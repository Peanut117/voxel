Return the needed voxel data read from the file.

Takes the name of the file.

Summary:
	Opens file;
	Read file header and version;
	Read the main chunk and its children using [[ReadMainChunk()]];
	Close file;
	Return [[Vox struct]]