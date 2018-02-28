# coding=utf-8

from __future__ import print_function
from __future__ import absolute_import
from dbfread import DBF
import os
import random
import time
import datetime
import pandas as pd
from collections import namedtuple
try:
    from kuanke.user_space_api import *
except:
    pass
from zipline.gens.type import BUY, SELL, LIMIT_CHARGE, FIVE_LEVEL_MARKET_ORDER
from zipline.gens.type import Order as TdxOrder
from zipline.gens.type import Position as TdxPosition
from zipline.gens.type import Portfolio as TdxPortfolio


class CatsTrade(object):
    """
    封装CATS的csv扫单接口
    """

    def __init__(self, account_type = "S0", account="000001", input_csv_dir="C:/CSVClientTrade/Input",
<<<<<<< HEAD
                 asset_dbf="C:/CSVClientTrade/Output/asset.dbf",
=======
                 asset_dbf="C:/CSVClientTrade/Output/assets.dbf",
>>>>>>> 830bc02f35f97300cf5798ef6ddbabb0f002bc60
                 order_dbf="C:/CSVClientTrade/Output/order_updates.dbf", order_sync=True):
        self.account = account
        self.account_type = account_type
        self.input_csv_dir = input_csv_dir
        self.asset_dbf = asset_dbf
        self.order_dbf = order_dbf
        self.order_sync = order_sync  # if in zipline, need order after order

    def _parse_assets(self):
        """
        assets.dbf 表结构:
        -- A_TYPE    资产类型。 P为股票持仓， F为资金
        -- ACCT_TYPE 账户类型，CATS系统里的账户类型，请根据不同账户类型填写，详情参见CATS API程序员手册，常见类型:股票集中交易: 0,   股票快速交易: F0,   信用集中交易: C, 信用快速交易: FC,   期货: A  (如果为空则取系统中已验证的默认账户的账户类型）
        -- ACCT      账户
        如果A_TYPE=F(资金）：
        s1	资金账户	暂时和acct一样，预留字段
        s2	币种	参见数据字典章节
        s3	当前余额
        s4	可用余额
        s5	当前保证金	期货账户时才有意义
        s6	冻结保证金	期货账户时才有意义
        如果A_TYPE=(持仓）：
        s1	标的代码	上海加后缀.SH，深圳加后缀.SZ, code
        s2	当前数量
        s3	可用数
        s4	成本价
        s5	持仓方向	期货账户时才有意义
        s6	投机套保标记	期货账户时才有意义
        s7	昨仓数	期货账户时才有意义

        :return:
        """
        if not os.path.exists(self.asset_dbf):
            return None, None

        db = DBF(self.asset_dbf)

        zp_portfolio = None
        positions = []
        positions_value = 0
        for rec in db:
            if rec["ACCT_TYPE"] != self.account_type or rec["ACCT"] != self.account:  # 多账号情况下
                continue
            if rec["A_TYPE"] == "F":
                zp_portfolio = TdxPortfolio(
                portfolio_value = float(rec["S3"]),
                cash = float(rec["S4"]),
                positions_value = 0
                )
            elif rec["A_TYPE"] == "P":
                pos = TdxPosition(
                sid = rec["S1"][:6],
                available = int(rec["S3"]),
                amount = int(rec["S2"]),
                cost_basis = float(rec["S4"]),
                last_sale_price = None,
                last_sale_date = None
                )
                positions.append(pos)
                positions_value += pos.amount * pos.cost_basis  # can't get last_sale_price, so it's so so OK

        zp_portfolio = TdxPortfolio(
            portfolio_value=zp_portfolio.portfolio_value,
            cash=zp_portfolio.cash,
            positions_value=positions_value
        )
        return zp_portfolio, positions

    def portfolio(self):
        zp_portfolio, positions = self._parse_assets()
        return zp_portfolio

    def positions(self):
        zp_portfolio, positions = self._parse_assets()
        return positions

    def _parse_order_update(self, client_id=None):
        """
        解析订单更新表：
        要求：在设置里面取消把所有订单流水一起输出
        client_id	char(8)	下单时带入的id
        ord_no	char(32)	订单编码
        ord_status	char(4)	订单状态	参见数据字典章节
        acct_type	char(8)	账户类型	参见数据字典章节
        acct	char(16)	交易账户
        cats_acct	char(16)	CATS账户	如果该交易账户有多个CATS账户绑定，可用于识别
        symbol	char(16)	标的代码
        tradeside	char(8)	交易方向	参见数据字典章节
        ord_qty	char(9)	委托数量
        ord_px	char(8)	委托价格
        ord_type	char(8)	委托类型	参见数据字典章节
        ord_param	char(32)	委托参数	某些类型的交易需要填写，默认为空
        corr_type	char(16)	下单来源类型	本面板下单固定为CLIENT_SCAN_ORDER@4
        corr_id	char(32)	下单来源ID
        filled_qty	char(9)	成交数量
        avg_px	char(8)	成交均价
        cxl_qty	char(9)	撤单数量
        ord_time	char(24)	委托时间	格式yyyy-MM-ddHH:mm:ss
        err_msg	char(32)	错误信息	如委托、撤单请求失败时有值

        对每一笔委托，服务端会合并计算最新的状态与数据，并推送至客户端，客户端收到数据时追加写入本文件(会根据订单号找到下单委托时的client_id，并填到相应字段。客户端重启后此前关联失效)
        注意：委托响应与流水推送都是异步机制，极端情况下可能有时序问题，即orderno与clientid对应之前，可能已经有成交回报回来。但一般情况下，流水会后到，因为委托响应是CATS后端收到请求处理后即返回，而流水推送需要报单到交易所再回来，链路时间一般相对长些。

        tradeside (交易方向)
        数值	含义
        1	买入
        2	卖出
        A	融资买入
        B	融券卖出
        C	买券还券
        D	卖券还款
        E	先买券还券，再担保品买入
        F	ETF申购
        G	ETF赎回
        FA	开多仓（开仓买入）
        FB	开空仓（开仓卖出）
        FC	平空仓（平仓买入）
        FD	平多仓（平仓卖出）

        order_status (订单状态)
        数值	含义
        0	已报
        1	部分成交
        2	全部成交
        3	部分撤单
        4	全部撤单
        5	交易所拒单
        6	柜台未接受

        currency (币种)
        数值	含义
        0	人民币
        1	美元
        2	港币

         

        acct_type(账户类型)
        数值	含义
        0	股票集中交易
        S0	股票模拟
        F0	股票深圳快速交易
        SHF0	股票上海快速交易
        C	信用集中交易
        FC	信用深圳快速交易
        SHFC	信用上海快速交易
        A	中信期货
        SA	期货模拟

        order_type(委托类型)
            数值	含义
        股票	0	限价单
            Q	市价单（对手方最优价格）
            R	市价单（最优五档即时成交剩余转限价）
            S	市价单（本方最优价格）
            T	市价单（即时成交剩余撤销）
            U	市价单（最优五档即时成交剩余撤销）
            V	市价单（全额成交或撤单）
        期货	0	限价单
            1	任意价

        :return:
        """
        if not os.path.exists(self.order_dbf):
            return None, None

        db = DBF(self.order_dbf, encoding="gbk")
        zf_orders = []
        order_dicts = dict()
        dt = datetime.datetime.today().strftime("%Y-%m-%d")
        for rec in db:
            if rec["ACCT_TYPE"] != self.account_type or rec["ACCT"] != self.account:  # 多账号情况下
                continue
            if rec["ORD_NO"] == "":
                continue
            if dt not in rec["ORD_TIME"]:  # 非今日订单
                continue
            order_id = rec["ORD_NO"]
            if order_id not in order_dicts:
                order_dicts[order_id] = rec
            else:
                old_write_time = pd.to_datetime(order_dicts[order_id]["WRITE_TIME"])
                new_write_time = pd.to_datetime(rec["WRITE_TIME"])
                old_status = int(order_dicts[order_id]["ORD_STATUS"])
                new_status = int(rec["ORD_STATUS"])
                if new_write_time > old_write_time or new_status > old_status:
                    order_dicts[order_id] = rec
            if client_id is not None and rec["CORR_ID"] == client_id:
                break

        for k, rec in order_dicts.items():
            zf_order = TdxOrder(
                dt=pd.to_datetime(rec["ORD_TIME"]),
                symbol=rec["SYMBOL"][:6],
                name=None,
                status=rec["ORD_STATUS"],
                price=float(rec["ORD_PX"]),
                amount=int(rec["ORD_QTY"]),
                order_id=rec["ORD_NO"],
                average_cost=float(rec["AVG_PX"]),
                filled=int(rec["FILLED_QTY"])
            )
            zf_orders.append(zf_order)
        return zf_orders

    def orders(self):
        zf_orders = self._parse_order_update()
        if zf_orders is None or len(zf_orders) <= 0:
            return None
        ret = dict()
        for od in zf_orders:
            ret[od.order_id] = od
        return ret

    def order(self, code, volume, price, action, order_type):
        if volume == 0:
            return
        if action == BUY:
            tradeside = "1"
        elif action == SELL:
            tradeside = "2"
        else:
            raise ValueError("action should be in BUY SELL")
        if order_type == LIMIT_CHARGE:
            cats_order_type = "0"
            if price is None or price <= 0:
                raise ValueError("price should be positive")
        elif order_type == FIVE_LEVEL_MARKET_ORDER:
            cats_order_type = "U"
        else:
            raise NotImplementedError("not implement for order type {}".format(order_type))

        cats_stock_code = code + ".SH" if code[0] == "6" else code + ".SZ"

        file_name = "csv_input_{}_{}.csv".format(time.mktime(datetime.datetime.now().timetuple()), int(random.random()*1000000))
        with open(os.path.join(self.input_csv_dir, file_name), 'w') as f:
            f.write(",".join(["O", self.account_type, self.account, cats_stock_code, str(volume), tradeside, "{:.2f}".format(price), cats_order_type]))
        if self.order_sync:
            max_try = 10
            retry = 0
            while True:
                time.sleep(0.2)
                orders = self._parse_order_update(file_name)
                if orders is not None and len(orders) > 0:
                    return orders[0], None
                retry += 1
                if retry >= max_try:
                    break
        return None, None

    def cancel_orders(self, exchange_id, order_id):
        self.cancel_order(order_id)

    def cancel_order(self, order_id):
        file_name = "csv_input_cancel_{}_{}.csv".format(time.mktime(datetime.datetime.now().timetuple()),
                                                 int(random.random() * 1000000))
        with open(os.path.join(self.input_csv_dir, file_name), 'w') as f:
            f.write(",".join(["C", self.account_type, self.account, order_id]))


if __name__ == "__main__":
    cats = CatsTrade(account="000002")
    cats.order("000002", 2500, 38, BUY, LIMIT_CHARGE)
    time.sleep(1)
    print(cats.orders())