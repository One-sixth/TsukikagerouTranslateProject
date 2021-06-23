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

parser.add_argument('--input_dir', required=True, type=str, help='包含snr_text文件的文件夹')
parser.add_argument('--output_dir', required=True, type=str, help='输出文件夹')
parser.add_argument('--baidu_appid', type=str, default='', help='百度翻译的APPID')
parser.add_argument('--baidu_key', type=str, default='', help='百度翻译的密钥')
parser.add_argument('--cache_file', type=str, default='cache_tr.json', help='翻译缓存')


args = parser.parse_args()


# input_dir = r'G:\月陽炎DVD\tmp4_unpack'
# output_dir = r'G:\月陽炎DVD\tmp4_unpack_trans'
input_dir = args.input_dir
output_dir = args.output_dir
trans_helper_appid = args.baidu_appid
trans_helper_key = args.baidu_key
cache_file = args.cache_file

# 关闭网络翻译标志
no_net_tr = False

if len(trans_helper_appid) == 0 and len(trans_helper_key) == 0:
    print('警告！参数 trans_helper_appid 或 trans_helper_key 没有设置，将关闭网络翻译')
    print(f'关闭网络翻译时，你需要手动修改翻译缓存来更新翻译。当前缓存文件为 {cache_file}')
    no_net_tr = True


os.makedirs(output_dir, exist_ok=True)

if no_net_tr:
    trans_helper = None
else:
    trans_helper = BaiduTranslator(appid=trans_helper_appid, key=trans_helper_key)
batch_trans_helper = BatchTranslationHelper(trans_helper, batch_size=2000, cache_file=cache_file)
pinyin_helper = Pinyin()
s2t_cc = OpenCC('s2t')


# def replace_to_X(exc):
#     '''
#     如果拼音都转换失败了，那就替换为X。
#     :param exc:
#     :return:
#     '''
#     return 'X'.encode('shiftjis'), exc.end


def replace_to_X(exc):
    '''
    如果都转换失败了，那就替换为X。
    :param exc:
    :return:
    '''
    return 'X'.encode('GB2312'), exc.end


# def zh_replace(exc):
#     '''
#     用于替换无法转换为shiftjis的中文字符
#     :param exc:
#     :return:
#     '''
#     wrong_char: str = exc.object[exc.start]
#     try:
#         # 尝试转换为繁体
#         t_wrong_char = s2t_cc.convert(wrong_char)
#         new_char = t_wrong_char.encode('shiftjis', errors='strict')
#     except UnicodeEncodeError:
#         # 繁体也不行，就使用拼音
#         pinyin_char = pinyin_helper.get_pinyin(wrong_char, tone_marks='numbers')
#         new_char = pinyin_char.encode('shiftjis', errors='replace_to_X')
#
#     return new_char, exc.end


# 注册替换函数
# codecs.register_error('zh_replace', zh_replace)
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

    out_fp = f'{output_dir}/{os.path.basename(in_fp)}'

    # 读取原文的文件
    snr_d = json.loads(open(in_fp, 'rb').read().decode('utf8'))

    if snr_d['file_type'] == 'snr_text':

        for b_i, bb in enumerate(snr_d['block_list']):
            text_data = bb['text_data']
            if len(text_data) == 0:
                continue

            text_data = base64.standard_b64decode(text_data)
            # 仅在找到shiftjis非ascii编码区的字符才进行转换
            if has_shiftjis_code(text_data):
                if text_data.find(b'\x00') != -1:
                    # 增加了分支块的多条文本的特殊处理
                    multi_text_data = text_data.split(b'\x00')
                    for t_i, t in enumerate(multi_text_data):
                        k = (b_i, t_i)
                        batch_trans_helper.add_in_item(k, t.decode('shiftjis'))
                else:
                    batch_trans_helper.add_in_item(b_i, text_data.decode('shiftjis'))

        # 批量翻译
        batch_trans_helper.do('jp', 'zh')
        # 翻译完，马上更新字典缓存，避免后面出错导致缓存都没了
        batch_trans_helper.update_cache_file()

        # 获得翻译结果
        out_items = batch_trans_helper.get_out_items()
        in_items = batch_trans_helper.get_in_items()

        # 分支块翻译缓存
        tmp_text_block_dict = {}

        # 更新翻译文件
        for b_i, r_text in out_items.items():
            if isinstance(b_i, tuple):
                # 更新分支块翻译缓存
                # l_text用来观察
                l_text = in_items[b_i]
                b_i, t_i = b_i
                tmp_text_block_dict.setdefault(b_i, {})
                tmp_text_block_dict[b_i][t_i] = (l_text, r_text)
            else:
                # 更新一般翻译块
                r_text: str
                # 额外处理，如果r_text长度为0，则增加一个空格
                if len(r_text) == 0:
                    r_text = ' '
                l_text = in_items[b_i]
                # f_str = r_text.encode('shiftjis', errors='zh_replace')
                # f_text = f_str.decode('shiftjis')
                f_str = r_text.encode('GB2312', errors='replace_to_X')
                f_text = f_str.decode('GB2312')

                print(f'{l_text}\n{r_text}\n{f_text}')

                f_base64 = base64.standard_b64encode(f_str).decode('ascii')
                snr_d['block_list'][b_i]['text_data'] = f_base64

        # 更新分支块翻译
        for b_i, td in tmp_text_block_dict.items():
            ks = sorted(list(td.keys()))
            l_texts = [td[k][0] for k in ks]
            r_texts = [td[k][1] for k in ks]
            # f_strs = [t.encode('shiftjis', errors='zh_replace') for t in r_texts]
            # f_texts = [t.decode('shiftjis') for t in f_strs]
            f_strs = [t.encode('GB2312', errors='replace_to_X') for t in r_texts]
            f_texts = [t.decode('GB2312') for t in f_strs]
            f_t = b'\x00'.join(f_strs)

            f_base64 = base64.standard_b64encode(f_t).decode('ascii')
            snr_d['block_list'][b_i]['text_data'] = f_base64

            for l_text, r_text, f_text in zip(l_texts, r_texts, f_texts):
                print(f'{l_text}\n{r_text}\n{f_text}')

        # 清除上次查询，不要忘记
        batch_trans_helper.clear()

    # 保存汉化的文件
    open(out_fp, 'wb').write(json.dumps(snr_d, ensure_ascii=True, indent=2, separators=(',', ': ')).encode('utf8'))
