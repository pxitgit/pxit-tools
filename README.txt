pxit-tools is a collection of two tools: pxit-encode and pxit-decode.

pxit-encode converts a single, arbritrary file into a set of TARGA image files.
pxit-decode converts a set of TARGA image files into a single file.

Pixel stream information transfer (pxit) is a method of transferring files by displaying synthetic images on television.

The makefile shows how to build both tools.  The sources were built using gcc:
		[frank@Arch src]$ gcc --version
		gcc (GCC) 10.1.0

The software is a minimal expression of the techniques used to represent digital information as a sequence of images.

NOTE: pxit can transfer any file but it doesn't preserve the filename.  We transfer compressed archives (7-zip) whose names are unimportant.

The software can be verified by the following steps:
1. Copy a file (e.g., BillOfRights.txt) into an empty directory
2. Create an archive named file.7z: 
			$7z a file BillOfRights.txt
			
3. Convert archive to TARGA files: 
			$pxit-encoder file.7z
			
4. Convert TARGA files back into an archive:
			$pxit-decoder . 
			
5. Find the new archive file, open it, and find the original text file.

