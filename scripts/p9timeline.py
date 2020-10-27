# -*- coding: UTF-8 -*-

from re import compile
from time import mktime
from urllib.request import urlopen
from sys import argv
from random import randint


reg_rank = compile('<a href="https?://psnine.com/psngame/([0-9]+)\\?psnid=(\\w+)">\\w+</a></p>\\s*'
                   '<em>.*?</em>\\s*</td>\\s*'
                   '<td width="12%" class="twoge">(.*?)<em>总耗时</em></td>')


reg_trophy = compile('<a href="https?://psnine.com/trophy/(\\d+)" class="text-([a-z]+)">.*?</a>'
                     '\\s*.*?</p>\\s*<em.*?</em>(?:\\s*<div.*?</div>)?\\s*</td>\\s*<td>\\s*'
                     '<em class="lh180 alert-success pd5 r" style="text-align:center;white-space:nowrap;" '
                     'tips="(\\d{4})年">\\s*(\\d{2})-(\\d{2})<br />(\\d{2}):(\\d{2})')


def get_trophy_player_list(game_id):
    url = 'http://psnine.com/psngame/' + str(game_id) + '/rank'
    with urlopen(url) as resp:
        res = resp.read().decode('utf-8')
        result = []
        for v in reg_rank.findall(res):
            if int(v[0]) != game_id:
                continue
            if v[2].endswith("个月"):
                result.append((v[1], 30.0 * float(v[2][:len(v[2]) - 2])))
            elif v[2].endswith("天"):
                result.append((v[1], float(v[2][:len(v[2]) - 1])))
            elif v[2].endswith("年"):
                result.append((v[1], 365.0 * float(v[2][:len(v[2]) - 1])))
        return result


def get_trophy_time_list(game_id, user_id):
    url = 'http://psnine.com/psngame/' + str(game_id) + '?psnid=' + user_id
    with urlopen(url) as resp:
        data = resp.read().decode('utf-8')
        matched = reg_trophy.findall(data)
    result = []
    for v in matched:
        sec = randint(0, 59)
        t = mktime((int(v[2]), int(v[3]), int(v[4]), int(v[5]), int(v[6]), sec, 0, 0, 0))
        result.append((int(v[0]) % 1000, v[1], int(t), '{}-{}-{} {}:{}:{:02}'.format(v[2], v[3], v[4], v[5], v[6], sec)))
    return result


if __name__ == '__main__':
    argc = len(argv)
    if argc < 3:
        print('Usage: python p9timeline.py <game_psn_id> <game_id_on_psnine> [player_id]')
        exit(-1)
    game_id = int(argv[2])
    if argc < 4:
        player_list = sorted(get_trophy_player_list(game_id), key=lambda x: x[1])
        time_list = get_trophy_time_list(game_id, player_list[10][0])
    else:
        time_list = get_trophy_time_list(game_id, argv[3])
    if time_list is None or len(time_list) == 0:
        print('Failed to get trophy list info from psnine!')
        exit(-1)
    time_list = sorted(time_list, key=lambda x: x[2])
    base_time = time_list[0][2]
    with open(argv[1] + '.txt', 'w') as f:
        for v in time_list:
            if v[1] != 'platinum':
                f.write('{},{},{},{},{}\n'.format(v[0], v[1], v[2] - base_time, v[2], v[3]))
