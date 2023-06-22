from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="datasus",
            sources=["datasus/decompress.c", "datasus/blast.c"],
            language="c",
        ),
    ],
    scripts=['scripts/dbc2dbf.py'],
)