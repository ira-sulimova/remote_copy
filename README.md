# Usage

### Start server 
```
./server
```

### Start client
```
./client <host the server is running on> <directory you wish to copy>
```

The client will create the folder you wish to copy on the local machine, and copy all the files & directories inside the remote directory into that folder. So, for example, if you wish to copy directory home/test/desired_directory, you would call client like this:

```
./client <host> home/test/desired_directory
```

The client will create a folder on the local machine:

```
./desired_directory
```

The program will copy all the contents of home/test/desired_directory into it. 
Client takes in only one directory as an argument. Run multiple clients for multiple directories. 

### Example client output:

```
-f ./desired_directory/target 
-f ./desired_directory/file4 
-f ./desired_directory/target.txt 
-d ./desired_directory/myfold 
-f ./desired_directory/myfold/super_in.txt 
-f ./desired_directory/has_stuff.txt 
-f ./desired_directory/other_big_file.txt 
```

For every folder or file copied, there’s an __-d__ and __-f__ flag, respectively, and the path to it on the local machine.

### Example server output: 

```
Request for /home/test/desired_directory
-f /home/test/desired_directory/target 
-f /home/test/desired_directory/file4 
-f /home/test/desired_directory/target.txt 
-d /home/test/desired_directory/myfold 
-f /home/test/desired_directory/myfold/super_in.txt 
-f /home/test/desired_directory/has_stuff.txt 
-f /home/test/desired_directory/other_big_file.txt 
```

The server prints this info for every folder or file sent, with paths on the remote machine.
