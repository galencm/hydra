from distutils.core import setup

setup(name='hydra',
	  description="""A protocol for sharing stuff""",
	  version='0.1',
	  packages=['hydra'],
	  package_dir={'': 'bindings/python'},
)
