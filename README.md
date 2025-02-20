# open-cbgm-standalone

![open-cbgm logo](https://github.com/jjmccollum/open-cbgm/blob/master/img/open-cbgm-logo.png)

[![Version 2.0.0](https://img.shields.io/badge/version-2.0.0-blue)](https://github.com/jjmccollum/open-cbgm-standalone)
[![Build Status](https://github.com/jjmccollum/open-cbgm-standalone/actions/workflows/testing.yml/badge.svg)](https://github.com/jjmccollum/open-cbgm-standalone/actions/workflows/testing.yml)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](https://choosealicense.com/licenses/mit/)

## About This Project

Standalone command-line interface for the open-cbgm library

### Introduction

This project provides a basic single-user interface for the open-cbgm library (https://github.com/jjmccollum/open-cbgm, [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4048498.svg)](https://doi.org/10.5281/zenodo.4048498)). In accordance with the open-cbgm library's expressed aim of TEI compliance, it processes TEI XML input files containing collation data and graph representations of local stemmata. For portability and efficiency, it calculates genealogical relationship data from this input once and caches it in a SQLite database (https://www.sqlite.org), which is stored in a file on the user's machine rather than in a separate server. Most of the scripts included in this interface are designed to interact with a populated SQLite database. Thanks to the efforts of @dopeyduck, the most parallelizable tasks are accelerated through multithreading.

SQLite boasts a relatively small memory footprint. A genealogical cache database generated by this interface will typically take several MB of space. The outputs of the graph-printing scripts are small `.dot` files (usually on the order of several KB each) containing rendering information for graphs.

SQLite databases also support fast reads and writes, and following the lead of Edmondson's Python implementation of the CBGM (https://github.com/edmondac/CBGM, [![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1296288.svg)](https://doi.org/10.5281/zenodo.1296288)), we have optimized our database's query performance with denormalization.

IMPORTANT UPDATE (2023/02/09): For those interested in a more user-friendly version of this interface, David Flood (@d-flood) has incorporated the `open-cbgm` core library and the scripts for this interface (along with many other useful features for processing TEI XML collations) into the Apatosaurus web app (https://apatosaurus.io/).

## Fast Installation (Pre-built Executables)

The binary executables for the open-cbgm standalone library are available for direct download in the latest release. Just click the latest link in the repository's "Releases" panel, and you will find compressed archives of the executables (each named according to its intended operating system) listed under "Assets" along with archives of the source code.

## Full Installation (Source Code) and Dependencies

To install the standalone interface on your system, clone this Git repository using the command

```
git clone git://github.com/jjmccollum/open-cbgm-standalone.git
```

You should now have the contents of the repository in an `open-cbgm-standalone` subdirectory of your current directory. From the command line, enter the new directory with the command

```
cd open-cbgm-standalone
```

The core open-cbgm library, along with its dependencies, is included as a Git submodule of this repository; if you do not have the submodule initialized, then you must initialize it with the command

```
git submodule update --init --recursive
```

Subsequently, if an update is made to the core library at a later time and you want to incorporate it, then enter the command

```
git submodule update --remote
```

Because the core library serializes graph outputs as `.dot` files, you will need to install graphviz (https://www.graphviz.org) to convert .dot files into image files. Platform-specific instructions for installing graphviz are provided below, and directions on how to get image files from the `.dot` outputs can be found in the "Usage" section below.

Once you have all dependencies in place, you need to build the project. The precise details of how to do this will depend on your operating system, but in all cases, you will need to have the CMake toolkit (https://cmake.org/) installed. We will provide platform-specific installation instructions below.

### Linux

To install CMake on Debian variants of Linux, the command is

```
sudo apt-get install cmake
```

Once you have CMake installed, you can complete the build with a few small lines. From the open-cbgm-standalone directory, create a build directory to store the build, enter it, and complete the build using the following commands:

```
mkdir build
cd build
cmake ..
make
```

Once these commands have executed, you should have all of the executable scripts added to the open-cbgm-standalone/build/scripts directory.

If graphviz is not installed on your system, then you can install it via the command

```
sudo apt-get install graphviz
```

### MacOS
    
The setup for MacOS is virtually identical to that of Linux. If you want to install CMake from the command line, it is recommended that you install the necessary packages with Homebrew (https://brew.sh/). If you are using Homebrew, simply enter the command

```
brew install cmake
```

Then, from the open-cbgm directory, enter the following commands:

```
mkdir build
cd build
cmake ..
make
```

Once these commands have executed, you should have all of the executable scripts added to the `open-cbgm-standalone/build/scripts` directory.

If graphviz is not installed on your system, then you can install it via the command

```
brew install graphviz
```

### Windows

If you want to install CMake from the command line, it is recommended that you install it with the Chocolatey package manager (https://chocolatey.org/). If you are using Chocolatey, simply enter the command

```
choco install -y cmake
```

If you do this, you'll want to make sure that your `PATH` environment variable includes the path to CMake. Alternatively, you can download the CMake user interface from the official website (https://cmake.org/) and use that.

How you proceed from here depends on whether you compile the code using Microsoft Visual Studio or a suite of open-source tools like MinGW. You can install the Community version of Microsft Visual Studio 2019 for free by downloading it from https://visualstudio.microsoft.com/vs/, or you can install it from the command line via

```
choco install -y visualstudio2019community
```

(Note that you will need to restart your computer after installing Visual Studio.) When you install Visual Studio, make sure you include the C++ Desktop Development workload necessary for building with CMake. If you install from the command line using Chocolatey, you can do this with the command

```
choco install -y visualstudio2019-workload-nativedesktop
```

You can install MinGW by downloading it from http://www.mingw.org/, or you can install it from the command line via

```
choco install -y mingw
```

If you are compiling with Visual Studio, then from the open-cbgm-standalone directory, enter the following commands:

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Once these commands have executed, you should have all of the executable scripts added to the `open-cbgm-standalone\build\scripts\Release` directory. Note that they will have the Windows `.exe` suffix, unlike the executables on Linux and MacOS.

If you are compiling with MinGW, then from the open-cbgm directory, enter the following commands:

```
mkdir build
cd build
cmake -DCMAKE_SH="CMAKE_SH-NOTFOUND" -G "MinGW Makefiles" ..
mingw32-make
```

Once these commands have executed, you should have all of the executable scripts added to the `open-cbgm-standalone\build\scripts` directory. Note that they will have the Windows `.exe` suffix, unlike the executables on Linux and MacOS.

If graphviz is not installed on your system, then you can install it using Chocolately via the command

```
choco install -y graphviz
```

Alternatively, you can download the library from https://www.graphviz.org/ and install it manually.

## Usage

When built, the standalone interface contains seven executable scripts: `populate_db`, `compare_witnesses`, `find_relatives`, `optimize_substemmata`, `print_local_stemma`, `print_textual_flow`, and `print_global_stemma`. The first script reads the input XML file containing collation and local stemma data and uses it to populate a local database from which the remaining scripts can quickly retrieve needed data. The second and third correspond to modules with the same names in both versions of the INTF's Genealogical Queries tool. The fourth offers functionality that is offered only partially or not at all in the Genealogical Queries tool. The fifth, sixth, and seventh generate graphs similar to those offered by the Genealogical Queries tool. We will provide usage examples and illustrations for each script in the subsections that follow. For these examples, we assume that you are executing all commands from the `bin` subdirectory of the `build` directory. The example commands appear as they would be entered on Linux and MacOS; for Windows, the executables will have the `.exe` suffix.

### Population of the Genealogical Cache

The `populate_db` script reads the input collation XML file, calculates genealogical relationships between all pairs of witnesses, and writes this and other data needed for common CBGM tasks to a SQLite database. Typically, this process will take at least a few minutes, depending on the number of variation units and witnesses in the collation, but the use of a database is intended to make this process one-time work. The script takes the input XML file as a required command-line argument, and it also accepts the following optional arguments for processing the data:
- `-t` or `--threshold`, which will set a threshold of minimum extant passages for witnesses to be included from the collation. For example, the argument `-t 100` will filter out any witnesses extant in fewer than 100 passages.
- `-z` followed by a reading type (e.g., `-z defective`), which will treat readings of that type as trivial for the purposes of witness comparison (so using the example already provided, a defective or orthographic subvariant of a reading would be considered to agree with that reading). This argument can be repeated with different reading types (e.g., `-z defective -z orthographic`).
- `-Z` followed by a reading type (e.g., `-Z lac`), which will ignore readings of that type, excluding them from variation units and local stemmata. Any witnesses having such readings will be treated as lacunose in the variation units where they have them. This argument can be repeated with different reading types (e.g., `-Z lac -Z ambiguous`).
- `-s` followed by a suffix for a witness siglum, which will ignore this suffix when it occurs with witness sigla in the collation. This argument can be repeated with different reading types (e.g., `-s "*" -s T -s V -s f`; note that the character `*`, which has a special function on the command line, must be placed between quotes).
- `--merge-splits`, which will treat split attestations of the same reading as equivalent for the purposes of witness comparison.
- `--classic`, which will use the "classic" CBGM rules for determining which readings explain others and how costs of genealogical relationships are calculated rather than using the open-cbgm library's standard rules. (For more details, see the "Substemma Optimization" subsection below.) 

Finally, please note that at this time, the current database must be overwritten, or a separate one must be created, in order to incorporate any changes to the processing options or to the local stemmata.

As an example, if we wanted to create a new database called cache.db using the `3_john_collation.xml` collation file in the core library's examples directory (which, for simplicity, we will assume we have copied to the directory from which we are executing the scripts), excluding ambiguous readings and witnesses with fewer than 100 extant readings, treating orthographic and defective subvariation as trivial, and using the classic CBGM genealogical calculations, then we would use the following command:

```
./populate_db -t 100 -z defective -z orthographic -Z ambiguous --classic 3_john_collation.xml cache.db
```

We note that the `3_john_collation.xml` file encodes lacunae implicitly (so that any witness not attesting to a specified reading is assumed to be lacunose), so lacunae do not have to be excluded explicitly. If we were using a minimally modified output from the ITSEE collation editor (https://github.com/itsee-birmingham/standalone_collation_editor), like the `john_6_23_collation.xml` example, then we would want to specify that readings of type `lac` be excluded:

```
./populate_db -t 100 -z defective -z orthographic -Z lac -Z ambiguous --classic john_6_23_collation.xml cache.db
```

To illustrate the effects of the processing arguments, we present several versions of the local stemma for the variation unit at 3 John 1:4/22–26, along with the commands used to populate the database containing their data. In the local stemmata presented below, dashed arrows represent edges of weight 0.

```
./populate_db 3_john_collation.xml cache.db
```

![3 John 1:4/22–26, no processing](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26-local-stemma.png)

```
./populate_db -z ambiguous -z defective 3_john_collation.xml cache.db
```

![3 John 1:4/22–26, ambiguous and defective readings trivial](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26-local-stemma-amb-def.png)

```
./populate_db -Z ambiguous --merge-splits 3_john_collation.xml cache.db
```

![3 John 1:4/22–26, ambiguous readings dropped and split readings merged](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26-local-stemma-drop-merge.png)

```
./populate_db -z defective -Z ambiguous --merge-splits 3_john_collation.xml cache.db
```

![3 John 1:4/22–26, ambiguous readings dropped, split readings merged, defective readings trivial](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26-local-stemma-drop-merge-def.png)

In the sections that follow, we will assume that the genealogical cache has been populated using the `-Z ambiguous` argument.

### Enumeration of Genealogical Relationships

The `enumerate_relationships` script produces a list of IDs for variation units where a given witness has a specific type of genealogical relationship with another given witness. This can be helpful if you want a quick way to check where two witnesses have, say, an unclear relationship in local stemmata. If our genealogical cache is stored in the database `cache.db` and we wanted to see all the variation units where the witness with number 5 has an unclear relationship with the witness with number 35, then we would enter the following command:

```
./enumerate_relationships cache.db 5 35 unclear
```

For convenience, we can also specify multiple types of desired genealogical relationships, and the script will print out the variation units at which they occur separately. So for separate lists of all the variation units where 5 is prior to 35 and all the variation units where 5 is posterior to 35, we would enter

```
./enumerate_relationships cache.db 5 35 prior posterior
```

If no type of genealogical relationship is specified, then separate lists will be specified for all of the allowed types of relationships: `extant` (places where neither witness is lacunose), `agree` (places where both witnesses agree), `prior` (places where the first witness is prior to the second), `posterior` (places where the first witness is posterior to the second), `norel` (places where the witnesses' readings are independent), `unclear` (places where the witnesses' readings have an unclear relationship), and `explained` (places where the first witness is explained by the second witness).

### Comparison of Witnesses

The `compare_witnesses` script is based on the "Comparison of Witnesses" module of the Genealogical Queries tool, but our implementation adds some flexibility. While the Genealogical Queries module only allows for the comparison of two witnesses at a time, `compare_witnesses` can compare a given witness with any number of other specified witnesses. If our genealogical cache is stored in the database `cache.db` and we wanted to compare the witness with number 5 with the witnesses with numbers 03, 35, 88, 453, 1611, and 1739, then we would enter the following command:

```
./compare_witnesses cache.db 5 03 35 88 453 1611 1739
```

The output of the script will resemble that of the Genealogical Queries module, but with additional rows in the comparison table. The rows are sorted in descending order of their number of agreements with the primary witness.

![Comparison of witness 5 with specified witnesses](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/compare_witnesses_5_03_35_88_453_1611_1739.png)

If we do not specify any other witnesses explicitly, then the script will compare the one witness specified with all other witnesses taken from the collation. So the command

```
./compare_witnesses cache.db 5
```

will return a table of genealogical comparisons between the witness with number 5 and all other collated witnesses, as shown below.

![Comparison of witness 5 with all other witnesses](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/compare_witnesses_5.png)

By default, the output table is written is fixed-width form to the console, but we can output the table in one of several forms (currently, fixed-width, CSV, TSV, and JSON are supported) using the `-f` flag, and we can write the output to a file using the `-o` flag. For instance, to write the table comparing witness 5 with witnesses 03, 35, 88, 453, 1611, and 1739 as a `.json` file, then we would use the following command:

```
./compare_witnesses -f json -o comparison.json cache.db 5 03 35 88 453 1611 1739
```

To exclude specific witnesses (e.g., P74 and 1241) from the comparison table, we can use the following command:

```
./compare_witnesses -e P74 -e 1241 cache.db 5
```

Similarly, to exclude all witnesses extant at fewer than 75% of variation units, we can use the following command:

```
./compare_witnesses -p 0.75 cache.db 5
```

The `-e` and `-p` options can be combined on the command-line.
They will be ignored if any secondary witnesses are specified.
Note that they may affect the ranks of the primary witness's potential ancestors (if former potential ancestors are excluded).

### Finding Relatives

The `find_relatives` script is based on the "Comparison of Witnesses" module of the Genealogical Queries tool, but our implementation adds some flexibility. For a given witness and variation unit address, the script outputs a table of genealogical comparisons between the given witness and all other collated witnesses, just like the `compare_witnesses` script does by default, but with an additional column indicating the readings of the other witnesses at the given variation unit. Following our earlier examples, if we want to list the relatives of witness 5 at 3 John 1:4/22–26 (whose number in the XML collation file is "B25K1V4U22-26"), then we would enter

```
./find_relatives cache.db 5 B25K1V4U22-26
```

This will produce an output like the one shown below.

![Relatives of witness 5 at 3 John 1:4/22–26](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/find_relatives_5.png)

Here, rather than filtering for witnesses by their numbers, we can filter for witnesses based on their variant reading IDs at the passage in question. We can specify one or more readings in the passage. For example, if we want to filter the results for just those supporting readings _d_ and _d2_ at this variation unit (which is how the Genealogical Queries module works by default), then we would add the reading IDs as additional arguments as follows:

```
./find_relatives cache.db 5 B25K1V4U22-26 d d2
```

This will produce the output shown below.

![Relatives of witness 5 with readings d and d2 at 3 John 1:4/22–26](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/find_relatives_5_rdg_d_d2.png)

Like the `compare_witnesses` script, this script can output its table in different formats as specified by the `-f` option and to specific output files as specified by the `-o` option. It can also support the `-e` option for the exclusion of specific witnesses as relatives and the `-p` option to exclude witnesses extant below a certain proportion of variation units as relatives.

### Substemma Optimization

In order to construct a global stemma, we need to isolate, for each witness, the most promising candidates for its ancestors in the global stemma. This process is referred to as _substemma optimization_. In order for a set of potential ancestors to constitute a feasible substemma for a witness, every extant reading in the witness must be explained by a reading in at least one of the potential ancestors. In order for a set of potential ancestors to constitute an optimal substemma for a witness, a particular objective function of the ancestors in the substemma must be minimized. 

The open-cbgm library uses a specially defined genealogical cost function for the purposes of substemma optimization. In the context of a local stemma, we define the _genealogical cost_ of one reading arising from another as the length of the shortest path from the prior reading to the posterior reading in the local stemma. If no path between the readings exists, then the cost is undefined. In the context of a witness and one of its potential ancestors, then the genealogical cost is defined as the sum of genealogical costs at every variation unit where they are defined. This can be further extended to substemmata, where the cost becomes the sum of the costs between the descendant and its stemmatic ancestors. In practice, substemmata with lower genealogical costs tend to exhibit more parsimony (i.e., they consist of few ancestors) and explain more of the witness's variants by agreement than other substemmata.

For genealogical relationships calculated using the `--classic` flag, different criteria govern which readings can be said to explain others and how the costs of substemma are calculated. In the classic formulation, a given reading can only be explained by a reading equivalent to it or directly prior to it, and the cost of a substemma between two witnesses is simply the number of passages where they are known to disagree. Note that in some rare circumstances, this rule set may result in a disconnected global stemma, with the traditional solution being the manual addition of intermediary nodes (Gerd Mink, "Problems of a Highly Contaminated Tradition: The New Testament: Stemmata of Variants as a Source of Genealogy for Witnesses," in Pieter van Reenen, August den Hollander, and Margot van Mulken, eds., _Studies in Stemmatology II_ \[Philadelphia, PA: Benjamins, 2004\], 13–85, esp. 59–63). The more relaxed criterion for explained readings adopted by the open-cbgm library avoids this scenario.

To get the optimal substemma for witness 5 in 3 John, we would enter the following command:

```
./optimize_substemmata cache.db 5
```

This will produce the output displayed below.

![Optimal substemma of witness 5 in 3 John](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/optimize_substemmata_5.png)

The `COST` column contains the total genealogical cost of the substemma. The `AGREE` column contains the total number of variants in the primary witness that can be explained by agreement with at least one stemmatic ancestor.

If the standard open-cbgm calculations are used, then this method is guaranteed to return at least one substemma that is both feasible and optimal, as long as the witness under consideration does not have equal priority to the _Ausgangstext_ (which typically only happens for fragmentary witnesses) and none of its readings have unclear sources. (Both problems can be solved by filtering out witnesses that are too lacunose and ensuring that all local stemmata have a single root reading.) In some cases, there may be multiple valid substemmata with the same cost, or there may be a valid substemma with a higher cost that we would consider preferable on other grounds (e.g., it has a significantly higher value in the `AGREE` column. If we have an upper bound on the costs of substemmata we want to consider, then we can enumerate all feasible substemmata within that bound instead. For instance, if we want to consider all feasible substemmata for witness 5 with costs at or below 10, then we would use the optional argument `-b 10` before the required argument as follows:

```
./optimize_substemmata -b 10 cache.db 5
```

This will produce the following output:

![Substemmata of witness 5 with costs within 10 in 3 John](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/optimize_substemmata_5_bound_10.png)

Be aware that specifying too high an upper bound may cause the procedure to take a long time.

Like the `compare_witnesses` script, this script can output its table in different formats as specified by the `-f` argument and to specific output files as specified by the `-o` argument. It also supports the `-e` option for the exclusion of specific witnesses as stemmatic ancestors and the `-p` option to exclude witnesses extant below a certain proportion of variation units as stemmatic ancestors.

### Generating Graphs

The two main steps in the iterative workflow of the CBGM are the formulation of hypotheses about readings in local stemmata and the evaluation and refinement of these hypotheses using textual flow diagrams. Ideally, the end result of the process will be a global stemma consisting of all witnesses and their optimized substemmata. The open-cbgm library has full functionality to generate textual graph description files for all diagrams used in the method.

In the standalone interface, all of this is accomplished with the `print_local_stemma`, `print_textual_flow`, and `print_global_stemma` scripts. The first requires at least one input (the genealogical cache database), but it can accept one or more variation unit identifiers as additional arguments, in which case it will only generate graphs for the local stemmata at those variation units. If no variation units are specified, then local stemmata will be generated for all of them. In addition, it accepts an optional `--weights` argument, which will add edge weights to the generated local stemma graphs. To generate the local stemma graph for 3 John 1:4/22–26, like the ones included above, you can use the command

```
./print_local_stemma cache.db B25K1V4U22-26
```

The `print_textual_flow` script accepts the same positional inputs (the database and, if desired, a list of specific variation units whose textual flow diagrams are desired), along with the following optional arguments indicating which specific graph types to generate:
- `--flow`, which will generate complete textual flow diagrams. A complete textual flow diagram contains all witnesses, highlighting edges of textual flow that involve changes in readings.
- `--attestations`, which will generate coherence in attestations textual flow diagrams for all readings in each variation unit. A coherence in attestations diagram highlights the genealogical coherence of a reading by displaying just the witnesses that support a given reading and any witnesses with different readings that are textual flow ancestors of these witnesses.
- `--variants`, which will generate coherence in variant passages textual flow diagrams. A coherence in variant passages diagram highlights just the textual flow relationships that involve changes in readings.

These arguments can be provided in any combination. If none of them is provided, then it is assumed that the user wants all graphs to be generated. In addition, a `--strengths` argument can be provided, which will format textual flow edges to highlight flow stability, per Andrew Edmondson's recommendation (Andrew Charles Edmondson, "An Analysis of the Coherence-Based Genealogical Method Using Phylogenetics" \[PhD diss., University of Birmingham, 2019\]). For example, to print just the complete textual flow diagram for 3 John 1:4/22–26 with flow stability highlighted, we would run the following command:

```
./print_textual_flow --flow --strengths cache.db B25K1V4U22-26
```

In addition, to override the default connectivity limit of the variation unit for the textual flow diagram (e.g., if these defaults were derived from the input XML file), a desired connectivity limit can be specified with the `-k` argument.
So to print the coherence in attestations diagram for 3 John 1:5/4 with a connectivity limit of 2, we would enter the following command:

```
./print_textual_flow --attestations -k 2 cache.db B25K1V5U4 
```

This script also supports the `-e` option for the exclusion of specific witnesses from the textual flow diagram and the `-p` option to exclude witnesses extant below a certain proportion of variation units from the textual flow diagram.

The `print_global_stemma` script requires at least one input (the database). It accepts an optional `--lengths` argument, which will label edges representing stemmatic ancestry relationships with their genealogical costs; this is not recommended unless the graph file is large enough to prevent crowding of edges and their labels. It also accepts an optional `--strengths` argument, which will highlight ancestry relationship edges according to their stability. It also supports the `-e` option for the exclusion of specific witnesses from the global stemma and the `-p` option to exclude witnesses extant below a certain proportion of variation units from the global stemma. This script optimizes the substemmata of all witnesses (choosing the first option in case of ties), then combines the substemmata into a global stemma. While this will produce a complete global stemma automatically, the resulting graph should be considered a "first-pass" result; users are strongly encouraged to run the `optimize_substemmata` script for individual witnesses and modify the graph according to their judgment.

The generated outputs are not image files, but `.dot` files, which contain textual descriptions of the graphs. To render the images from these files, we must use the `dot` program from the graphviz library. As an example, if the graph description file for the local stemma of 3 John 1:4/22–26 is `B25K1V4U22-26-local-stemma.dot`, then the command

```
dot -Tpng -O B25K1V4U22-26-local-stemma.dot
```

will generate a PNG image file called `B25K1V4U22-26-local-stemma.dot.png`. (If you want to specify your own output file name, use the `-o` argument followed by the file name you want.)

Sample images of local stemmata have already been included at the beginning of the "Usage" section. For the sake of completeness, we have included sample images of the other types of graphs below.

Complete textual flow diagram for 3 John 1:4/22–26:
![3 John 1:4/22–26 complete textual flow diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26-textual-flow.png)

Coherence in attestations diagrams for all readings in 3 John 1:4/22–26:
![3 John 1:4/22–26a coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26Ra-coherence-attestations.png)
![3 John 1:4/22–26af coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26Raf-coherence-attestations.png)
![3 John 1:4/22–26b coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26Rb-coherence-attestations.png)
![3 John 1:4/22–26c coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26Rc-coherence-attestations.png)
![3 John 1:4/22–26d coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26Rd-coherence-attestations.png)
![3 John 1:4/22–26d2 coherence in attestations diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26Rd2-coherence-attestations.png)

Coherence in variant passages diagram for 3 John 1:4/22–26:
![3 John 1:4/22–26 coherence in variant passages diagram](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/B25K1V4U22-26-coherence-variants.png)

Complete global stemma for 3 John (multiple roots are due to fragmentary witnesses and readings with unclear sources):
![3 John global stemma](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/global-stemma.png)

Complete global stemma for 3 John, with fragmentary witnesses (fewer than 100 extant passages) excluded and all local stemmata completed:
![3 John global stemma](https://github.com/jjmccollum/open-cbgm-standalone/blob/master/images/global-stemma-connected.png)
