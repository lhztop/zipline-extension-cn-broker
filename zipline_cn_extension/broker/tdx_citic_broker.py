# encoding: UTF-8

from __future__ import print_function
from __future__ import absolute_import
from zipline.gens.brokers.tdx_shipane_broker import TdxShipaneBroker
from six import iteritems
from zipline import protocol
from zipline.finance.execution import (
    MarketOrder,
    LimitOrder,
    StopOrder,
    StopLimitOrder
)
from zipline.finance.order import (
    Order as ZPOrder,
    ORDER_STATUS as ZP_ORDER_STATUS
)
from zipline.gens.type import Transaction as TdxTransaction
from zipline.gens.type import Order as TdxOrder
from zipline.gens.type import Position as TdxPosition
from zipline.gens.type import Portfolio as TdxPortfolio
from zipline.gens.type import OrderRt
from zipline.finance.transaction import Transaction as ZPTransaction
from zipline.api import symbol
from zipline.gens.type import *
import datetime
from logbook import Logger
import pandas as pd
from tdx.engine import Engine
import numpy as np
import zerorpc
import platform
from zipline.errors import SymbolNotFound
try:
    from ..trade.citic import CatsTrade
except:
    from trade.citic import CatsTrade

log = Logger("TDX_citic_broker")


class TdxCatsBroker(TdxShipaneBroker):
    """
    中信Cats系统CSV下单模块封装
    """

    def __init__(self, cats_client=None):
        """

        :param cat_client:
        :type cat_client: CatsTrade
        """
        self._shipane_client = cats_client
        self._orders = {}
        self.currency = 'RMB'
        self._subscribed_assets = []
        self._bars = {}
        self._bars_update_dt = None
        self._bars_update_interval = pd.tslib.Timedelta('5 S')
        self._mkt_client = Engine(auto_retry=True, best_ip=True)
        self._mkt_client.connect()
        # super(TdxShipaneBroker, self).__init__(tdx_uri, account_id)

    def tdx_order_to_zipline_order(self, order):
        """
        status
        0	已报
        1	部分成交
        2	全部成交
        3	部分撤单
        4	全部撤单
        5	交易所拒单
        6	柜台未接受

        :param order:
        :return:
        """
        if order.status == "3" or '4' == order.status:
            zp_status = ZP_ORDER_STATUS.CANCELLED
        elif order.status == "0":
            zp_status = ZP_ORDER_STATUS.OPEN
        elif order.status == "1":
            zp_status = ZP_ORDER_STATUS.FILLED
        elif order.status == "2":
            zp_status = ZP_ORDER_STATUS.HELD
        elif order.status == "5" or order.status == "6":
            zp_status = ZP_ORDER_STATUS.REJECTED


        zp_order_id = self._tdx_to_zp_order_id(order.order_id)

        od = ZPOrder(
            dt=order.dt,
            asset=symbol(order.symbol),
            amount=order.amount,
            filled=order.filled,
            stop=None,
            limit=order.price,  # TODO 市价单和限价单
            id=zp_order_id,
        )
        od.broker_order_id = order.order_id
        od.status = zp_status

        return od

if __name__ == "__main__":
    cats_trader = CatsTrade()
    broker = TdxCatseBroker(cats_client=cats_trader)
    print(broker.orders)
    print(broker.positions)
    print(broker.portfolio)

    Asset = namedtuple("Assert",["symbol"])
    broker.order(Asset(symbol = "000002"), 2100, LimitOrder(limit_price=38))
    print(broker.orders)
    print(broker.positions)

    broker.order(Asset(symbol = "000002"), 2500, LimitOrder(limit_price=35))
    print(broker.orders)
    print(broker.positions)
    orders = broker.orders
    for k, v in orders.items():
        broker.cancel_order(v.order_id)
    print(broker.orders)
