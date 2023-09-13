import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        if platform.system() == "Windows":
            cmake_version = LooseVersion(re.search(r'version\s*([\d.]+)', out.decode()).group(1))
            if cmake_version < '3.1.0':
                raise RuntimeError("CMake >= 3.1.0 is required on Windows")

        self.install_dependencies()

        for ext in self.extensions:
            self.build_extension(ext)

    def install_dependencies(self):
        dir_start = os.getcwd()
        dir_pybind11 = os.path.join(dir_start, 'pybind11')
        if os.path.exists(dir_pybind11):
            return 0
        os.mkdir(dir_pybind11)
        subprocess.check_call(['git', 'clone',
                               'https://github.com/pybind/pybind11.git',
                               dir_pybind11])
        #subprocess.check_call(['git', 'submodule', 'update', '--init', '--recursive'])

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        # required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                      '-DPYTHON_EXECUTABLE=' + sys.executable]

        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        if platform.system() == "Windows":
            cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
            if sys.maxsize > 2**32:
                cmake_args += ['-A', 'x64']
            build_args += ['--', '/m']
        else:
            cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

setup(
    name='pyflagsercount',
    version='0.4.0',
    author='Jason P. Smith',
    author_email='jasonsmith.bath@gmail.com',
    description='A package for counting directed cliques in directed graphs',
    long_description= 'A program for counting directed cliques in directed graphs, adapted from https://github.com/luetge/flagser\n\nOfficial source code repo: https://github.com/JasonPSmith/flagser-count\n\nRequirements: numpy ≥ 1.17.0 and cmake version ≥ 3.9.',
    url='https://github.com/JasonPSmith/flagser-count',
    ext_modules=[CMakeExtension('pyflagsercount')],
    cmdclass=dict(build_ext=CMakeBuild),
    packages=["pyflagsercount"],
    install_requires=['numpy>=1.17.0','cmake>=2.8.12'],
    zip_safe=False,
)
