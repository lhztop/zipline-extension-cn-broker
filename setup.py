from setuptools import setup
from setuptools import find_packages
from distutils.extension import Extension
import platform

try:
    from Cython.Build import cythonize
except ImportError:
    def cythonize(extensions): return extensions
    sources = ['src/trade/_citic.cpp']
else:
    sources = ['src/trade/_citic.pyx']

sources.append('src/trade/jsoncpp.cpp')
sources.append('src/trade/CiticTradeWrapper.cpp')

import os.path
base_trade_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "src", "trade")
print(base_trade_path)
base_hq_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "src", "hq")
print(base_trade_path)
print(__file__)

extra_compile_args=[
        '-std=c++11',
        '-O3',
        '-Wall',
        # '-Wextra',
        # '-Wconversion',
        '-fno-strict-aliasing'
    ]

extra_link_args = []
if platform.system() == 'Windows':
    extra_compile_args.append("/Gy")
    extra_compile_args.append("/MD")
    extra_link_args.append("/LD")
    extra_link_args.append("/Gy")


libraries = [
        'CITICs_HsT2Hlp'
    ]
# if platform.system() == "Linux" and "64" in platform.machine():
#     libraries.append('t2sdk64')
# else:
#     libraries.append('t2sdk')

mod1 = Extension(
    'src.trade._citic',
    sources,
    include_dirs = [base_trade_path, base_hq_path],
    library_dirs = [base_trade_path, base_hq_path],
    extra_compile_args = extra_compile_args,
    extra_link_args = extra_link_args,
    language='c++',
    libraries=libraries
)

setup(
    name="zipline-extension-cn-broker",
    version='0.1.0',
    description="Python for Trader wrapper in china",
    keywords='zipline broker china',
    author='Hongzhong Li',
    author_email="Use the github issues",
    url="https://github.com/lhztop/zipline-extension-cn-broker.git",
    license='MIT License',
    install_requires=['setuptools'],
    package_dir={'zipline-extension-cn-broker': 'src'},
    ext_modules=cythonize([mod1]),
    setup_requires=['pytest-runner'],
    tests_require=['pytest'],
    include_package_data=True
)
