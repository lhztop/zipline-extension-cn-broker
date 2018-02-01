# coding=utf-8

import cython
from libcpp.string cimport string
from libcpp cimport bool as cpp_bool
from cpython cimport array
import array

cdef extern from "CiticTradeWrapper.h":
    cdef cppclass CiticTradeWrapper:
        string ConfigFile "ConfigFile"
        cpp_bool Login()
        cpp_bool System()
        string Portfolio()
        string Positions()
        string Order(const char* stock_code, float price, int volume)
        string Transactions()
        string Orders()
        string CancelOrder(const char* orderId)

        CiticTradeWrapper(const char* account, const char* password, const char * configFile = "Hsconfig.ini", const char* op_entrust_way = "3", const char* clientName = "ZZJJ")
