```
rsync -av rsync://cvs.sv.gnu.org/sources/global global
mkdir cvs2git-tmp && cvs2git --blobfile=cvs2git-tmp/git-blob.dat --dumpfile=cvs2git-tmp/git-dump.dat --fallback-encoding=ascii --username=cvs2git global/global
git init --bare global.git
cd global.git
cat ../cvs2git-tmp/git-blob.dat ../cvs2git-tmp/git-dump.dat | git fast-import
git push --mirror origin
```

http://lists.gnu.org/archive/html/bug-global/
http://lists.gnu.org/archive/html/help-global/
http://lists.gnu.org/archive/html/info-global/


https://www.gnu.org/software/automake/manual/html_node/Basics-of-Distribution.html

pygments http://lists.gnu.org/archive/html/bug-global/2016-06/msg00004.html http://lists.gnu.org/archive/html/bug-global/2016-06/msg00005.html