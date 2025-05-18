Returns void;

Takes:
* File
* [[Vox struct]]*

Summary:
	Read the chunk header and check if this is MAIN;
	If the chunk is valid, read all the children with [[ReadChildChunks()]]