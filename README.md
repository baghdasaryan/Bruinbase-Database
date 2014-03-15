Bruinbase-Database
==================

Contributors:
-------------
| Name                | Email                             |
| ----                | -----                             |
| Georgi Baghdasaryan | baghdasaryan@ucla.edu             |
| Michael Sweatt      | mickeysweatt@engineering.ucla.edu |

Project Description
-------------------
Implementation of an example database called Bruinbase-Database that 
uses is capable of using B+tree indexes for query processing.

Currently, this program supports only bulk LOAD (with and without
indexing) and SELECT queries.

Usage
-----
First, we need to download and build the project. Follow the following steps
to do so:

```shell
$ git clone https://github.com/baghdasaryan/Bruinbase-Database.git
$ cd Bruinbase-Database
$ make
```

`make` will build the executable file, `bruinbase`, in the current
directory.

Now, you can start the program:
```shell
$ ./bruinbase
```

Once you are inside Bruinbase-Database, you can interact with it by issuing SELECT, LOAD,
and QUIT commands. All tables in Bruinbase-Database have two columns, key (integer) and
value (string of length up to 99). For example, the Movie table that has been
preloaded to Bruinbase can be queried as follows:

```
Bruinbase> select * from movie where key > 1000 and key < 1010
1008 'Death Machine'
1002 'Deadly Voyage'
1004 'Dear God'
1003 'Deal of a Lifetime'
1009 'Death to Smoochy'
  -- 0.162 seconds to run the select command. Read 403 pages

Bruinbase> select count(*) from movie
3616
  -- 0.160 seconds to run the select command. Read 403 pages

Bruinbase> select * from movie where key=3421
3421 'Remember the Titans'
  -- 0.162 seconds to run the select command. Read 403 pages

Bruinbase> select * from movie where value='Die Another Day'
1074 'Die Another Day'
  -- 0.162 seconds to run the select command. Read 403 pages

Bruinbase> 
```

Note that the first column of the table is an integer column with the name key
and the second column is a string column with the name value.

As in the above example, Bruinbase-Database supports simple SELECT statements. You can
list only one table name in the FROM clause. You can have one of:
* key
* value
* \*
* COUNT(\*)
in the SELECT clause. You can list multiple conditions in the WHERE clause, but
all conditions should be ANDed together. OR is not supported. All basic
comparison operators (<, <=, >, >=, =, <>) can be used as part of the
conditions. The table and column names are case insensitive, so movie and MOVIE
refer to the same table.

Bruinbase-Database also supports a bulk load command that can be used to load
data into a table from a file. Syntax to load data into a table is
```
LOAD tablename FROM 'filename' [ WITH INDEX ]
```

This command creates a table named tablename and loads the (key, value) pairs
from the file filename. If the option WITH INDEX is specified, Bruinbase also
creates the index on the key column of the table. The format for the input file
must be a single key and value pair per line, separated by a comma. The key must
be an integer, and the value (a string) should be enclosed in double quotes,
such as:

1,"value 1"
2,"value 2"
...
See *movie.del* file for an example

Examples of load command:
```
LOAD movie FROM 'movie.del'
LOAD indexedMovie FROM 'movie.del' WITH INDEX
```
Once the load completes, you should be able to run SELECT queries, as described above.

Once you are done, you can issue the QUIT command to exit:
```
Bruinbase> quit
```

