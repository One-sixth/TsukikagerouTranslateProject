import json
import os
from typing import Callable
import shutil


class BatchTranslationHelper:
    '''
    批量翻译助手，把多句文本拼成一大块的文本，一次性发出翻译请求，大大增加了翻译速度。
    同时提供翻译缓存功能，避免相同文本重复提交翻译
    '''

    def __init__(self, translator: Callable, batch_size=2000, batch_line=100, cache_file=None):
        '''
        :param translator: 网络翻译器
        :param batch_size: 每批的最大字数
        :param batch_line: 每批的最大函数
        :param cache_file: 翻译缓存文件，建议使用
        '''
        self.translator = translator
        self.batch_size = batch_size
        self.batch_line = batch_line
        self.cache_file = cache_file

        if translator is None:
            print('警告！网络翻译已关闭')

        '''
        字典使用3级结构
        src_lang->dst_lang->query->result
        '''
        self.cache_dict = {}

        if self.cache_file is not None and os.path.isfile(self.cache_file):
            self.cache_dict = json.loads(open(self.cache_file, 'rb').read().decode('utf8'))
            assert isinstance(self.cache_dict, dict)

        self.in_items = {}
        self.out_items = {}

    def add_in_item(self, key, line: str):
        '''
        增加一行待翻译文本
        :param key: 引用该翻译文本的Key
        :param line:待翻译文本
        :return:
        '''
        assert isinstance(line, str)
        assert line.find('\x0A') == -1, '错误！不允许内容有回车符'
        assert line.find('\x0D') == -1, '错误！不允许内容有回车符'
        assert line.find('\x00') == -1, '错误！不允许内容有00字符'
        assert len(line) <= self.batch_size, '错误！不允许一行的大小超过一批次的大小'

        if key in self.in_items:
            print('警告！Key已存在，将会被替换')

        self.in_items[key] = line

    def do(self, src_lang, dst_lang, *, batch_size=None, batch_line=None, translator_kwargs=None):
        '''
        进行批量翻译
        :param src_lang: 源语言
        :param dst_lang: 目标语言
        :param batch_size: 一批的最大字符数量
        :param batch_line: 一批最大行数
        :param translator_kwargs: 额外的翻译器的参数
        :return:
        '''
        # 设定字典默认值
        if translator_kwargs is None:
            translator_kwargs = dict()
        self.cache_dict.setdefault(src_lang, {})
        self.cache_dict[src_lang].setdefault(dst_lang, {})

        # 获得当前翻译缓存字典
        cur_tr_dict = self.cache_dict[src_lang][dst_lang]

        # 批量额外设定
        if batch_size is None:
            batch_size = self.batch_size

        if batch_line is None:
            batch_line = self.batch_line

        batchs = []

        keys = []
        big_text = ''

        for key, value in list(self.in_items.items()):
            if value in cur_tr_dict:
                # 如果条目已查询过
                r = cur_tr_dict[value]
                self.out_items[key] = r
            else:
                # 生成批量
                assert len(value) + 1 < batch_size, '错误，单个条目超过最大批量大小'

                # 如果超过批量大小，则截断
                if len(big_text) + len(value) + 1 >= batch_size or len(keys) > batch_line:
                    batchs.append([keys, big_text])
                    keys = []
                    big_text = ''

                keys.append(key)
                if len(big_text) == 0:
                    big_text += value
                else:
                    big_text += '\n' + value

        if len(keys) > 0:
            batchs.append([keys, big_text])
            # keys = []
            # big_text = ''
        # 删掉引用，避免误用
        del keys, big_text

        for keys, in_big_text in batchs:
            in_texts = in_big_text.split('\n')
            if self.translator is not None:
                # 使用网络翻译
                out_big_text = self.translator(src_text=in_big_text, src_lang=src_lang, dst_lang=dst_lang, **translator_kwargs)
                out_texts = out_big_text.split('\n')
            else:
                # 关闭网络翻译后，翻译结果将会变成空字符串
                out_texts = [''] * len(keys)
            assert len(out_texts) == len(keys), '错误！长度不一致，翻译出错？'
            for k, lv, rv in zip(keys, in_texts, out_texts):
                self.out_items[k] = rv
                cur_tr_dict[lv] = rv

    def get_out_items(self):
        '''
        获得输出字典
        :return:
        '''
        return self.out_items

    def get_in_items(self):
        '''
        获得输入字典
        :return:
        '''
        return self.in_items

    def clear(self):
        '''
        清除所有输入和输出
        :return:
        '''
        self.in_items = {}
        self.out_items = {}

    def cache_clear_empty_result(self):
        '''
        清除翻译缓存中所有的空翻译
        :return:
        '''
        n = 0
        for a in self.cache_dict.values():
            for b in a.values:
                for k, c in b.items():
                    if len(c) == 0:
                        del b[k]
                        n+=1
        return n

    def update_cache_file(self):
        '''
        刷新缓存文件
        :return:
        '''
        if self.cache_file is None:
            print('警告！缓存文件路径为None，将会被忽略')
        else:
            # 更新缓存时，先备份缓存，避免缓存损坏
            if os.path.isfile(self.cache_file):
                bak_cache_file = self.cache_file+'.bak'
                shutil.copy(self.cache_file, bak_cache_file)
            open(self.cache_file, 'wb').write(json.dumps(self.cache_dict, ensure_ascii=False, indent=2, separators=(',', ': ')).encode('utf8'))


if __name__ == '__main__':
    from baidu_translator import BaiduTranslator
    trans1 = BaiduTranslator()

    trans2 = BatchTranslationHelper(trans1, 5000, cache_file='batch_trans.json')

    trans2.add_in_item(1, '苹果')
    trans2.add_in_item(2, '梨子')
    trans2.add_in_item(3, '西瓜')
    trans2.add_in_item(4, '天上掉馅饼')
    trans2.add_in_item(5, '夏天好乘凉')

    trans2.do('zh', 'jp')

    out_items = trans2.get_out_items()
    print(out_items)
    # trans2.update_cache_file()

