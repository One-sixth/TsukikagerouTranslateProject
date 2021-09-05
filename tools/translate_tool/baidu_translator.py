'''
百度翻译API文档
http://api.fanyi.baidu.com/product/113
'''

import time
from urllib import parse
import hashlib
import requests
import random
import json


class BaiduTranslator:
    '''
    百度翻译工具，使用的是免费的通用翻译接口，需要appid和key才能使用。
    文本太长会导致丢数据，要注意
    免费版百度翻译1秒只能接受一次请求，所以需要批量翻译工具复杂处理，不然几百万行文本要翻译到猴年马月。
    '''

    def __init__(self, appid: str=None, key: str=None, *, retry=10):
        assert appid is not None and key is not None, '错误！appid 和 key 参数不能为空'
        assert len(appid) > 0 and len(key) > 0
        self.appid = appid
        self.key = key
        self.retry = retry
        self.api = 'https://fanyi-api.baidu.com/api/trans/vip/translate'

    @staticmethod
    def _make_sign(appid, q, salt, key):
        s1 = f'{appid}{q}{salt}{key}'
        md5 = hashlib.md5()
        md5.update(s1.encode('utf8'))
        return md5.hexdigest()

    def __call__(self, src_text: str, src_lang: str, dst_lang: str, raise_on_fail=True):
        # 对于多行文本，api会返回很多个结果，但是顺序和原来的却不一定意义，而且还会自动消除行前行后的空格符。
        # 重复行还会自动唯一化处理
        src_lines = src_text.split('\n')
        # 去除头尾的所有空格符
        src_lines = [l.strip() for l in src_lines]
        r_dict = dict(zip(src_lines, [None]*len(src_lines)))

        q = '\n'.join(r_dict.keys())

        salt = random.randint(1, 65535)
        sign = self._make_sign(self.appid, q, salt, self.key)

        # 构建POST表单
        form_data = {
            'q': q,
            'from': str(src_lang),
            'to': str(dst_lang),
            'appid': str(self.appid),
            'salt': str(salt),
            'sign': str(sign),
        }

        is_success = False

        # 默认重试10次
        for i in range(self.retry):
            try:
                # 对于多行数据，api那边会自动分割多行
                # 发现丢数据，原因未知，现在需要进阶处理
                r = requests.post(url=self.api, data=form_data, headers={'Content-Type': 'application/x-www-form-urlencoded'})
                if r.status_code == 200:
                    d = json.loads(r.text)
                    if 'trans_result' not in d:
                        raise RuntimeError(f'Translation failed. Because {r.text}')
                    # 这里有个假设，返回的结果跟输入的顺序是一致的
                    for i, p2 in enumerate(d['trans_result']):
                        r_dict[parse.unquote(p2['src'], encoding='utf8')] = parse.unquote(p2['dst'], encoding='utf8')
                    is_success = True
                    break
                else:
                    raise RuntimeError(f'Translation failed. Because status code is {r.status_code}')
            except Exception as e:
                print(str(e))
                # time.sleep(random.randint(1, 3))
                time.sleep(1)

        if not is_success and raise_on_fail:
            raise RuntimeError(f'Translation failed. After 10 attempts, it still fails.')

        for p2 in r_dict.items():
            if len(p2[0]) > 0 and p2[1] is None:
                if raise_on_fail:
                    raise RuntimeError('Error! The input text is too long. Please shorten the length of the input text.')
                else:
                    print('Warning! The input text is too long. Untranslated lines will be replaced with blank lines.')
                    p2[1] = ''

        dst_text = '\n'.join([r_dict[l] for l in src_lines])

        return dst_text


if __name__ == '__main__':
    helper = BaiduTranslator(None, None)
    print(helper('苹果', 'zh', 'jp'))
    print(helper('苹果\n雪梨', 'zh', 'jp'))
