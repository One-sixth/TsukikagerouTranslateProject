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
from opencc import OpenCC
from xpinyin import Pinyin
from baidu_translator import BaiduTranslator
from batch_translation_helper import BatchTranslationHelper


parser = ArgumentParser(description="月阳炎自动翻译工具")

parser.add_argument('--input_dir', required=True, type=str, help='包含原始文件的文件夹')
parser.add_argument('--output_dir', required=True, type=str, help='输出文件夹')
parser.add_argument('--cache_file', type=str, default='cache_tr.json', help='翻译缓存')


args = parser.parse_args()


# input_dir = r'D:\github_repo\TsukikagerouTranslateProject\月阳炎\3-2.月阳炎-导出的文本-原始'
# output_dir = r'D:\github_repo\TsukikagerouTranslateProject\月阳炎\3-2.月阳炎-导出的文本-翻译后'
# cache_file = r'D:\github_repo\TsukikagerouTranslateProject\cache_tr.json'
input_dir = args.input_dir
output_dir = args.output_dir
cache_file = args.cache_file


os.makedirs(output_dir, exist_ok=True)


tr_cache = json.load(open(cache_file, 'r', encoding='utf8'))
tr_cache_jp_zh = tr_cache['jp']['zh']


# 这里使用一个文件翻译，一个文件保存策略。
for in_fp in glob.glob(f'{input_dir}/*.snr.json'):

    out_fp = f'{output_dir}/{os.path.basename(in_fp)}'

    # 跳过已有的文本
    if os.path.isfile(out_fp):
        print(f'跳过已有的翻译后文件，避免被覆盖。已跳过{out_fp}')
        continue

    # 读取原文的文件
    block_list = json.load(open(in_fp, 'r', encoding='utf8'))
    # 不是分支块的，直接单行文本
    # 是分支块的，使用列表

    for b_i, block in enumerate(block_list):
        if block is None:
            continue
        elif isinstance(block, list):
            for b_i2, text2 in enumerate(block):
                block_list[b_i][b_i2] = tr_cache_jp_zh.get(text2, text2)
        elif isinstance(block, str):
            block_list[b_i] = tr_cache_jp_zh.get(block, block)
        else:
            raise AssertionError()

    # 保存汉化的文件
    json.dump(block_list, open(out_fp, 'w', encoding='utf8'), ensure_ascii=False, indent=2)
