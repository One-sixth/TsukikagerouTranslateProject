'''
导出每个剧本里的文件到单个文件中，单独逐个翻译
'''

'''
月阳炎翻译工具

注意，输入输出字符集均为shiftjis
很多简体字在shiftjis中没有对应字符，目前使用这样的降级方式，避免出现无法显示字符
简体字检查->繁体字检查->拼音->字符X

使用缓存字典，避免失败时重复请求已请求过的内容

编码base64时，记得注意长度，末尾的=字符不可省略，c++的base64转换函数有长度的强制检查

注意额外检查，如果text中没有shiftjis的编码，则跳过该段翻译

学会改字符集了！！！不用再转换成shiftjis编码了！！！直接上GB2312！！！给力！！！
'''

import os
import glob
import json
import base64
import codecs
from argparse import ArgumentParser


parser = ArgumentParser(description="月阳炎自动翻译工具")

parser.add_argument('--input_dir', required=True, type=str, help='包含snr_text文件的文件夹')
parser.add_argument('--output_dir', required=True, type=str, help='输出文件夹')


args = parser.parse_args()


# input_dir = r'D:\github_repo\TsukikagerouTranslateProject\月阳炎\3.月阳炎-导出的文本'
# output_dir = r'D:\github_repo\TsukikagerouTranslateProject\月阳炎\3-2.月阳炎-导出的文本-原始'
input_dir = args.input_dir
output_dir = args.output_dir


os.makedirs(output_dir, exist_ok=True)


def replace_to_X(exc):
    '''
    如果都转换失败了，那就替换为X。
    :param exc:
    :return:
    '''
    return 'X'.encode('GB2312'), exc.end


# 注册替换函数
codecs.register_error('replace_to_X', replace_to_X)


def has_shiftjis_code(s: bytes):
    '''
    检查字符串里面是否含有shiftjis的片假名，平假名，汉字，标点符号。

    Shift_JIS编码字节结构

    以下字符在Shift_JIS使用一个字节来表示。

    ASCII字符（0x20-0x7E），但“\”被“¥”取代
    ASCII控制字符（0x00-0x1F、0x7F）
    JIS X 0201标准内的半角标点及片假名（0xA1-0xDF）
    在部分操作系统中，0xA0用来放置“不换行空格”。

    以下字符在Shift_JIS使用两个字节来表示。

    JIS X 0208字集的所有字符

    “第一位字节”使用0x81-0x9F、0xE0-0xEF（共47个）
    “第二位字节”使用0x40-0x7E、0x80-0xFC（共188个）

    使用者定义区

    “第一位字节”使用0xF0-0xFC（共13个）
    “第二位字节”使用0x40-0x7E、0x80-0xFC（共188个）

    在Shift_JIS编码表中，并未使用0xFD、0xFE及0xFF。

    在微软及IBM的日语电脑系统中，在0xFA、0xFB及0xFC的两字节区域，加入了388个JIS X 0208没有收录的符号和汉字。

    :param s:
    :return:
    '''
    on_first = False

    found_shiftjis = False

    for b in s:
        if on_first:
            # 目前是第二个字节
            on_first = False
            if 0x40 <= b <= 0x7E or 0x80 <= b <= 0xFC:
                found_shiftjis = True
                break
        else:
            # 目前是第一个字节
            if 0x81 <= b <= 0x9F or 0xE0 <= b <= 0xEF:
                on_first = True
            elif 0xA1 <= b <= 0xDF:
                found_shiftjis = True
                break

    return found_shiftjis


# 这里使用一个文件翻译，一个文件保存策略。
for in_fp in glob.glob(f'{input_dir}/*.snr'):
    block_list = []

    out_fp = f'{output_dir}/{os.path.basename(in_fp)}.json'

    # 读取原文的文件
    snr_d = json.loads(open(in_fp, 'rb').read().decode('utf8'))

    if snr_d['file_type'] == 'snr_text':

        out_block_list = []
        # 不是分支块的，直接单行文本
        # 是分支块的，使用列表

        for b_i, bb in enumerate(snr_d['block_list']):
            text_data = bb['text_data']
            if len(text_data) == 0:
                out_block_list.append(None)
                continue

            text_data = base64.standard_b64decode(text_data)
            # 仅在找到shiftjis非ascii编码区的字符才进行转换
            if has_shiftjis_code(text_data):
                if text_data.find(b'\x00') != -1:
                    # 增加了分支块的多条文本的特殊处理
                    multi_text_data = text_data.split(b'\x00')
                    multi_text_data = [t.decode('shiftjis') for t in multi_text_data]
                    out_block_list.append(multi_text_data)
                else:
                    out_block_list.append(text_data.decode('shiftjis'))
            else:
                out_block_list.append(None)

    # 保存汉化的文件
    json.dump(out_block_list, open(out_fp, 'w', encoding='utf8'), ensure_ascii=False, indent=2)
