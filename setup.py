from distutils.core import setup, Extension

setup(
    name = 'ht_file',
    version = '1.0',
    description = 'Manage Filepath Hashtables',
    author = 'Humantech',
    author_email = 'jean.schmidt@humantech.com.br',
    long_description = '''
Manages filepath hashtables, export/inport it in b64 and check if filepath
is in a given hashtable
''',
    ext_modules = [
        Extension("ht_file", ["py_integration.cpp"])
    ]
)