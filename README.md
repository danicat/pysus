# DATASUS library for Python

A library to open DATASUS files (*.dbc) in Python

***IMPORTANT***: this project is still in an experimental state. Use it at your own risk. Actually, even after it matures, you still will have to use it at your own risk :) (see [LICENSE](LICENSE) for details)

## Usage notes

Since this is a development version you will need to install the package from this repo. Future versions will be published to PyPI as soon as I achieve my goal which is to have a simple function to read DBC files to a Pandas dataframe.

To install it to your system you can use `make`:

```sh
$ make install
```

Note: this requires `setuptools` to be installed on your system.

### Reading a *.dbc file to a Pandas DataFrame

```python
from datasus import read_dbc
import pandas as pd

df = pd.DataFrame(read_dbc("my-file.dbc"))
```

### Decompressing *.dbc files to *.dbf 

You can decompress a *.dbc file from python importing the `decompress` method:

```sh
$ python3
>>> from datasus import decompress
>>> decompress("file.dbc", "file.dbf")
>>>
```

Or you can run the helper script from the command line:

```sh
$ dbc2dbf.py input-file.dbc output-file.dbf
```

## Contact

For support, kudos and complaints drop me an e-mail at daniela.petruzalek@gmail.com. Or Twitter @danicat83, at least until Elon burns the place to the ground.

## I don't like Python (why???)

If you don't like Python and prefer R instead, see my other project [read.dbc](https://github.com/danicat/read.dbc).