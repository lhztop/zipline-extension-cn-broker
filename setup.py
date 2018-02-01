from setuptools import setup
from setuptools import find_packages
from distutils.extension import Extension
import platform

try:
    from Cython.Build import cythonize
except ImportError:
    def cythonize(extensions): return extensions
    sources = ['zipline_cn_extension/trade/_citic.cpp']
else:
    sources = ['zipline_cn_extension/trade/_citic.pyx']

sources.append('zipline_cn_extension/trade/jsoncpp.cpp')
sources.append('zipline_cn_extension/trade/CiticTradeWrapper.cpp')

import os.path
base_trade_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "zipline_cn_extension", "trade")
print(base_trade_path)
base_hq_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "zipline_cn_extension", "hq")
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

if "32" in platform.architecture()[0]:
    mod1 = Extension(
        'zipline_cn_extension.trade._citic',
        sources,
        include_dirs = [base_trade_path, base_hq_path],
        library_dirs = [base_trade_path, base_hq_path],
        extra_compile_args = extra_compile_args,
        extra_link_args = extra_link_args,
        language='c++',
        libraries=libraries
    )
else:
    mod1 = None

setup(
    name="zipline-cn-extension",
    version='0.1',
    description="Python for zipline trader/broker wrapper in china",
    keywords='zipline broker china',
    author='Hongzhong Li',
    author_email="lhztop@qq.com",
    url="https://github.com/lhztop/zipline-extension-cn-broker.git",
    license='MIT License',
    install_requires=['setuptools'],
    package_dir={'zipline-extension-cn-broker': 'zipline_cn_extension'},
    packages=find_packages(include=['zipline_cn_extension', 'zipline_cn_extension.*']),
    ext_modules=cythonize([mod1]) if mod1 is not None else None,
    setup_requires=['pytest-runner'],
    tests_require=['pytest'],
    package_data={root.replace(os.sep, '.'):
                  ['*.pyi', '*.pyx', '*.pxi', '*.pxd','*.pyd']
                  for root, dirnames, filenames in os.walk('zipline_cn_extension')
                  if '__pycache__' not in root},
    include_package_data=True
)
