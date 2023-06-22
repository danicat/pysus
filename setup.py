from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="datasus",
            sources=["src/decompress.c", "src/blast.c"],
            language="c",
        ),
    ]
)