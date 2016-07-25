#!/usr/bin/python2
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This file is a utility to quickly flash boards

import argparse
import collections
import fcntl
import os
import select
import subprocess as sp
import time
from copy import deepcopy
from abc import ABCMeta, abstractmethod

# For most tests, error codes should never conflict
CTS_CONFLICTING_CODE = -1
CTS_SUCCESS_CODE = 0
TH_BOARD = 'stm32l476g-eval'
OCD_SCRIPT_DIR = '/usr/local/share/openocd/scripts'
MAX_SUITE_TIME_SEC = 3

class Board(object):
  """Class representing a single board connected to a host machine

  This class is abstract, subclasses must define the updateSerial()
  method

  Attributes:
    board: String containing actual type of board, i.e. nucleo-f072rb
    config: Directory of board config file relative to openocd's
        scripts directory
    hla_serial: String containing board's hla_serial number (if board
    is an stm32 board)
    tty_port: String that is the path to the tty port which board's
        UART outputs to
    _tty_descriptor: String of file descriptor for tty_port
  """

  configs = {
      'stm32l476g-eval': 'board/stm32l4discovery.cfg',
      'nucleo-f072rb': 'board/st_nucleo_f0.cfg'
  }

  __metaclass__ = ABCMeta  # This is an Abstract Base Class (ABC)
  def __init__(self, board, hla_serial=None, flash_offset='0x08000000'):
    """Initializes a board object with given attributes

    Args:
      board: String containing board name
      hla_serial: Serial number if board's adaptor is an HLA
    """
    self.board = board
    self.hla_serial = hla_serial
    self.tty_port = None
    self._tty_descriptor = None
    self.flash_offset = flash_offset

  @abstractmethod
  def updateSerial(self):
    """Subclass should implement this"""
    pass

  def sendOpenOcdCommands(self, commands):
    """Send a command to the board via openocd

    Args:
      commands: A list of commands to send
    """
    args = ['openocd', '-s', OCD_SCRIPT_DIR,
        '-f', Board.configs[self.board], '-c', 'hla_serial ' + self.hla_serial]

    for cmd in commands:
      args += ['-c', cmd]
    args += ['-c', 'shutdown']
    sp.call(args)

  def make(self, module, ec_dir):
    """Builds test suite module for board"""
    cmds = ['make',
        '--directory=' + ec_dir,
        'BOARD=' + self.board,
        'CTS_MODULE=' + module,
        '-j',
        '-B']

    print 'EC directory is ' + ec_dir
    print 'Building module \'' + module + '\' for ' + self.board
    sp.call(cmds)

  def flash(self):
    """Flashes board with most recent build ec.bin"""
    flash_cmds = [
        'reset_config connect_assert_srst',
        'init',
        'reset init',
        'flash write_image erase build/' +
            self.board +
            '/ec.bin ' +
            self.flash_offset,
        'reset']

    self.sendOpenOcdCommands(flash_cmds)

  def toString(self):
    s = ('Type: Board\n'
       'board: ' + self.board + '\n'
       'hla_serial: ' + self.hla_serial + '\n'
       'config: ' + Board.configs[self.board] + '\n'
       'tty_port ' + self.tty_port + '\n'
       '_tty_descriptor: ' + str(self._tty_descriptor) + '\n')
    return s

  def reset(self):
    """Reset board (used when can't connect to TTY)"""
    self.sendOpenOcdCommands(['init', 'reset init', 'resume'])

  def setupForOutput(self):
    """Call this before trying to call readOutput for the first time.
    This is not in the initialization because caller only should call
    this function after serial numbers are setup
    """
    self.updateSerial()
    self.reset()
    self._identifyTtyPort()

    # In testing 3 retries is enough to reset board (2 can fail)
    num_file_setup_retries = 3
    # In testing, 10 seconds is sufficient to allow board to reconnect
    reset_wait_time_seconds = 10
    try:
      self._getDevFileDescriptor()
    # If board was just connected, must be reset to be read from
    except (IOError, OSError):
      for i in range(num_file_setup_retries):
        self.reset()
        time.sleep(reset_wait_time_seconds)
        try:
          self._getDevFileDescriptor()
          break
        except (IOError, OSError):
          continue
    if self._tty_descriptor is None:
      raise ValueError('Unable to read ' + self.name + '\n'
               'If you are running cat on a ttyACMx file,\n'
               'please kill that process and try again')

  def readAvailableBytes(self):
    """Read info from a serial port described by a file descriptor

    Return:
      Bytes that UART has output
    """
    buf = []
    while True:
      if select.select([self._tty_descriptor], [], [], 1)[0]:
        buf.append(os.read(self._tty_descriptor, 1))
      else:
        break
    result = ''.join(buf)
    return result

  def _identifyTtyPort(self):
    """Saves this board's serial port"""
    dev_dir = '/dev/'
    id_prefix = 'ID_SERIAL_SHORT='
    num_reset_tries = 3
    reset_wait_time_s = 10
    com_devices = [f for f in os.listdir(
      dev_dir) if f.startswith('ttyACM')]

    for i in range(num_reset_tries):
      for device in com_devices:
        self.tty_port = os.path.join(dev_dir, device)
        properties = sp.check_output(['udevadm',
                        'info',
                        '-a',
                        '-n',
                        self.tty_port,
                        '--query=property'])
        for line in [l.strip() for l in properties.split('\n')]:
          if line.startswith(id_prefix):
            if self.hla_serial == line[len(id_prefix):]:
              return
      if i != num_reset_tries - 1: # No need to reset the obard the last time
        self.reset() # May need to reset to connect
        time.sleep(reset_wait_time_s)

    # If we get here without returning, something is wrong
    raise RuntimeError('The device dev path could not be found')

  def _getDevFileDescriptor(self):
    """Read available bytes from device dev path"""
    fd = os.open(self.tty_port, os.O_RDONLY)
    flag = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flag | os.O_NONBLOCK)
    self._tty_descriptor = fd

class TestHarness(Board):
  """Subclass of Board representing a Test Harness

  Attributes:
    serial_path: Path to file containing serial number
  """

  def __init__(self, serial_path=None):
    """Initializes a board object with given attributes

    Args:
      serial_path: Path to file containing serial number
    """
    Board.__init__(self, TH_BOARD)
    self.serial_path = serial_path

  def updateSerial(self):
    """Loads serial number from saved location"""
    if self.hla_serial:
      return # serial was already loaded
    try:
      with open(self.serial_path, mode='r') as ser_f:
        self.hla_serial = ser_f.read()
        return
    except IOError:
      msg = ('Your th hla_serial may not have been saved.\n'
           'Connect only your th and run ./cts --setup, then try again.')
      raise RuntimeError(msg)

  def saveSerial(self):
    """Saves the th serial number to a file

    Return: the serial number saved
    """
    serial = Cts.getSerialNumbers()
    if len(serial) != 1:
      msg = ('TH could not be identified.\n'
           '\nConnect your TH and remove other st-link devices')
      raise RuntimeError(msg)
    else:
      ser = serial[0]
      if not ser:
        msg = ('Unable to save serial')
        raise RuntimeError(msg)
      if not os.path.exists(os.path.dirname(self.serial_path)):
        os.makedirs(os.path.dirname(self.serial_path))
      with open(self.serial_path, mode='w') as ser_f:
        ser_f.write(ser)
        self.hla_serial = ser
        return ser

class DeviceUnderTest(Board):
  """Subclass of Board representing a DUT board

  Attributes:
    th: Reference to test harness board to which this DUT is attached
  """

  def __init__(self, board, th, hla_ser=None, f_offset='0x08000000'):
    """Initializes a Device Under Test object with given attributes

    Args:
      board: String containing board name
      th: Reference to test harness board to which this DUT is attached
      hla_serial: Serial number if board uses an HLA adaptor
    """
    Board.__init__(self, board, hla_serial=hla_ser, flash_offset=f_offset)
    self.th = th

  def updateSerial(self):
    """Stores the DUT's serial number.

    Precondition: The DUT and TH must both be connected, and th.hla_serial
    must hold the correct value (the th's serial #)
    """
    if self.hla_serial != None:
      return # serial was already set ('' is a valid serial)
    serials = Cts.getSerialNumbers()
    dut = [s for s in serials if self.th.hla_serial != s]
    if  len(dut) == 1:
      self.hla_serial = dut[0]
      return # Found your other st-link device serial!
    else:
      raise RuntimeError('Your TH serial number is incorrect, or your have'
          ' too many st-link devices attached.')
    # If len(dut) is 0 then your dut doesn't use an st-link device, so we
    # don't have to worry about its serial number

class Cts(object):
  """Class that represents a CTS testing setup and provides
  interface to boards (building, flashing, etc.)

  Attributes:
    dut: DeviceUnderTest object representing dut
    th: TestHarness object representing th
    module: Name of module to build/run tests for
    ec_directory: String containing path to EC top level directory
    test_names: List of strings of test names contained in given module
    test_results: Dictionary of results of each test from module, with
        keys being test name strings and values being test result integers
    return_codes: List of strings of return codes, with a code's integer
      value being the index for the corresponding string representation
  """

  def __init__(self, ec_dir, dut='nucleo-f072rb', module='gpio'):
    """Initializes cts class object with given arguments.

    Args:
      dut: Name of Device Under Test (DUT) board
      ec_dir: String path to ec directory
      module: Name of module to build/run tests for
    """
    self.results_dir = '/tmp/cts_results'
    self.ec_directory = ec_dir
    self.th = TestHarness()
    self.dut = DeviceUnderTest(dut, self.th)  # DUT constructor needs TH

    th_ser_path = os.path.join(
        self.ec_directory,
        'build',
        self.th.board,
        'th_hla_serial')

    self.module = module

    testlist_path = os.path.join(
        self.ec_directory,
        'cts',
        self.module,
        'cts.testlist')

    self.test_names = Cts._getMacroArgs(testlist_path, 'CTS_TEST')

    self.th.serial_path = th_ser_path

    return_codes_path = os.path.join(self.ec_directory,
        'cts',
        'common',
        'cts.rc')

    self.return_codes = Cts._getMacroArgs(
        return_codes_path, 'CTS_RC_')

    self.test_results = collections.OrderedDict()

  def set_module(self, mod):
    """Sets the module instance variable. Also sets test_names,
    since that depends directly on the module we are using

    Args:
      mod: String of module name
    """
    self.module = mod


  def make(self):
    self.dut.make(self.module, self.ec_directory)
    self.th.make(self.module, self.ec_directory)

  def flashBoards(self):
    """Flashes th and dut boards with their most recently build ec.bin"""
    self.updateSerials()
    self.th.flash()
    self.dut.flash()

  def setup(self):
    """Saves th serial number if th only is connected.

    Return:
      Serial number that was saved
    """
    return self.th.saveSerial()

  def updateSerials(self):
    """Updates serials of both th and dut, in that order (order matters)"""
    self.th.updateSerial()
    self.dut.updateSerial()

  def resetBoards(self):
    """Resets the boards and allows them to run tests
    Due to current (7/27/16) version of sync function,
    both boards must be rest and halted, with the th
    resuming first, in order for the test suite to run
    in sync
    """
    self.updateSerials()
    self.th.sendOpenOcdCommands(['init', 'reset halt'])
    self.dut.sendOpenOcdCommands(['init', 'reset halt'])
    self.th.sendOpenOcdCommands(['init', 'resume'])
    self.dut.sendOpenOcdCommands(['init', 'resume'])

  @staticmethod
  def getSerialNumbers():
    """Gets serial numbers of all st-link v2.1 board attached to host

    Returns:
      List of serials
    """
    usb_args = ['lsusb', '-v', '-d', '0x0483:0x374b']
    usb_process = sp.Popen(usb_args, stdout=sp.PIPE, shell=False)
    st_link_info = usb_process.communicate()[0]
    st_serials = []
    for line in st_link_info.split('\n'):
      if 'iSerial' in line:
        st_serials.append(line.split()[2].strip())
    return st_serials

  @staticmethod
  def _getMacroArgs(filepath, macro):
    """Get list of args of a certain macro in a file when macro is used
    by itself on a line

    Args:
      filepath: String containing absolute path to the file
      macro: String containing text of macro to get args of
    """
    args = []
    with open(filepath, 'r') as fl:
      for ln in [ln for ln in fl.readlines(
      ) if ln.strip().startswith(macro)]:
        ln = ln.strip()[len(macro):]
        args.append(ln.strip('()').replace(',', ''))
    return args

  def _parseOutput(self, r1, r2):
    """Parse the outputs of the DUT and TH together

    Args;
      r1: String output of one of the DUT or the TH (order does not matter)
      r2: String output of one of the DUT or the TH (order does not matter)
    """
    self.test_results.clear()  # empty out any old results

    for output_str in [r1, r2]:
      for ln in [ln.strip() for ln in output_str.split('\n')]:
        tokens = ln.split()
        if len(tokens) != 2:
          continue
        print 'Tokens are: ' + str(tokens)
        test = tokens[0].strip()
        try:
          return_code = int(tokens[1])
        except ValueError: # Second token is not an int
          continue
        if test not in self.test_names:
          continue
        elif self.test_results.get(
            test,
            CTS_SUCCESS_CODE) == CTS_SUCCESS_CODE:
          self.test_results[test] = return_code
          print 'Is ' + str(return_code) + ' the same as ' + str(self.test_results[test])
        elif return_code != self.test_results[test]:
          print 'Setting ' + test + ' to CTS_CONFLICTING_CODE'
          self.test_results[test] = CTS_CONFLICTING_CODE

  def _resultsAsString(self):
    """Takes saved results and returns a duplicate of their dictionary
    with the return codes replaces with their string representation

    Returns:
      dictionary with test name strings as keys and test result strings
          as values
    """
    result = deepcopy(self.test_results)
    # Convert codes to strings
    for test, code in result.items():
      if code == CTS_CONFLICTING_CODE:
        result[test] = 'RESULTS CONFLICT'
      else:
        result[test] = self.return_codes[code]

    for tn in self.test_names:
      if tn not in result:
        # Exceptional case
        result[tn] = 'NO RESULT RETURNED'

    return result

  def prettyResults(self):
    """Takes saved results and returns a string representation of them

    Return: Dictionary similar to self.test_results, but with strings
        instead of error codes
    """
    res = self._resultsAsString()
    t_long = max(len(s) for s in res.keys())
    e_max_len = max(len(s) for s in res.values())

    pretty_results = 'CTS Test Results for ' + self.module + ' module:\n'

    for test, code in res.items():
      align_str = '\n{0:<' + str(t_long) + \
        '} {1:>' + str(e_max_len) + '}'
      pretty_results += align_str.format(test, code)

    return pretty_results

  def resetAndRecord(self):
    """Resets boards, records test results in results dir"""
    self.updateSerials()
    self.dut.setupForOutput()
    self.th.setupForOutput()

    self.dut.readAvailableBytes()  # clear buffer
    self.th.readAvailableBytes()
    self.resetBoards()

    time.sleep(MAX_SUITE_TIME_SEC)

    dut_results = self.dut.readAvailableBytes()
    th_results = self.th.readAvailableBytes()
    if not dut_results or not th_results:
      raise ValueError('Output missing from boards.\n'
               'If you are running cat on a ttyACMx file,\n'
               'please kill that process and try again')

    self._parseOutput(dut_results, th_results)
    pretty_results = self.prettyResults()

    dest = os.path.join(self.results_dir, self.dut.board, self.module + '.txt')
    if not os.path.exists(os.path.dirname(dest)):
      os.makedirs(os.path.dirname(dest))

    with open(dest, 'w') as fl:
      fl.write(pretty_results)

    print pretty_results

def main():
  """Main entry point for cts script from command line"""
  ec_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
  os.chdir(ec_dir)

  dut_board = 'nucleo-f072rb'  # nucleo by default
  module = 'gpio'  # gpio by default

  parser = argparse.ArgumentParser(description='Used to build/flash boards')
  parser.add_argument('-d',
            '--dut',
            help='Specify DUT you want to build/flash')
  parser.add_argument('-m',
            '--module',
            help='Specify module you want to build/flash')
  parser.add_argument('-s',
            '--setup',
            action='store_true',
            help='Connect only the th to save its serial')
  parser.add_argument('-b',
            '--build',
            action='store_true',
            help='Build test suite (no flashing)')
  parser.add_argument('-f',
            '--flash',
            action='store_true',
            help='Flash boards with most recent image and record results')
  parser.add_argument('-r',
            '--reset',
            action='store_true',
            help='Reset boards and save test results (no flashing)')

  args = parser.parse_args()

  if args.module:
    module = args.module

  if args.dut:
    dut_board = args.dut

  cts_suite = Cts(ec_dir, module=module, dut=dut_board)

  if args.setup:
    serial = cts_suite.setup()
    print 'Your th hla_serial # has been saved as: ' + serial

  elif args.reset:
    cts_suite.resetAndRecord()

  elif args.build:
    cts_suite.make()

  elif args.flash:
    cts_suite.flashBoards()

  else:
    cts_suite.make()
    cts_suite.flashBoards()

if __name__ == "__main__":
  main()
