#!/usr/bin/env python3
"""Render the prespecified candidate; refuse incomplete or changed results."""
import contextlib, hashlib, io, json
from pathlib import Path
import development_table

ROOT = Path(__file__).resolve().parent
EXPECTED = {
    'tyhh_qos_protect_relay': 'bc17533408b781019adf1cd2128e0e284dd0b482accb6272bb8178692c6b3ef2',
    'tyhh_qos_relay_counter': 'b6997ce15649203ad4254e622ef9510136afb468831c758acad232e0f0b1c7f3',
    'V3_portal': 'b5ff885fdebd3bcc3a30b2c986d5e02dafecccca98ca3ac15ddfc2e53ad3e856',
    'xxqztest': '7846b40b8f734889fd014b5f361f4f719f1898835c546488c172f878dcb40822',
}
OPS = ('V3_portal', 'xxqztest')

def main():
    data = json.loads((ROOT/'final_verification.json').read_text())
    assert data['seeds'] == [74001,74002,74003]
    base, new = data['runs']
    assert [base['bot'],new['bot']] == ['tyhh_qos_protect_relay','tyhh_qos_relay_counter']
    for run in (base,new):
        assert run['complete_and_valid'] and run['verified_replays'] and len(run['rows']) == 120
        for name, manifest in run['source_manifests'].items():
            assert hashlib.sha256(json.dumps(manifest,sort_keys=True).encode()).hexdigest() == EXPECTED[name]
    success = all(new['by_opponent'][o]['win_rate'] > 0.5 for o in OPS)
    verdict = '独立样本达到双对手过半；不等于逐图全胜或稳定优势。' if success else '未实现同时战胜两个对手的目标，不能将开发集改善表述为最终成功。'
    out = ['# Relay 双对手实验报告','','日期：2026-09-29。','',verdict,'',
           '候选在最终评测前已冻结为tyhh_qos_relay_counter；未根据最终成绩重新挑选。原relay与两个对手均未改动。',
           '每版本120局，每对手60局：十张官方地图，三个新种子74001/74002/74003，交换A/B。种子是随机整数，不是日期。','',
           '| 对手 | 原版胜/负/平 | counter胜/负/平 | 胜率变化 |','|---|---:|---:|---:|']
    for o in OPS:
        b,c = base['by_opponent'][o],new['by_opponent'][o]
        out.append(f"| {o} | {b['wins']}/{b['losses']}/{b['draws']} ({b['win_rate']:.1%}) | {c['wins']}/{c['losses']}/{c['draws']} ({c['win_rate']:.1%}) | {(c['win_rate']-b['win_rate'])*100:+.1f}个百分点 |")
    out += ['','## 种子波动与配对变化','','| 对手 | 版本 | 各种子胜场/20 | 胜率均值 ± 样本标准差 |','|---|---|---|---|']
    for o in OPS:
        for run in (base,new):
            q = run['by_opponent'][o]
            counts = ', '.join(f"{s}: {v['wins']}" for s,v in q['by_seed'].items())
            out.append(f"| {o} | {run['bot']} | {counts} | {q['mean_seed_win_rate']*100:.1f}% ± {q['sample_sd']*100:.1f}个百分点 |")
    for o in OPS:
        p = new['paired_to_baseline'][o]
        out.append(f"\n{o}：原版负→新版胜{p['loss_to_win']}局，原版胜→新版负{p['win_to_loss']}局。\n")
    out += ['','三个种子只作描述统计，未计算p值，不宣称统计显著或稳定统治。','','## 各图完整结果','','| 地图 | V3原版→counter胜场/6 | xxqztest原版→counter胜场/6 |','|---|---:|---:|']
    for m in base['by_opponent'][OPS[0]]['by_map']:
        cells = [f"{base['by_opponent'][o]['by_map'][m]['wins']}/6 → {new['by_opponent'][o]['by_map'][m]['wins']}/6" for o in OPS]
        out.append(f'| {m} | {cells[0]} | {cells[1]} |')
    out += ['','## 全部开发结果','','开发600局与最终240局分离；失败版本全部保留。选择规则、时点及源码树哈希见FREEZE.md。','']
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf): development_table.main()
    out += [buf.getvalue().strip(),'','## 完整性与资料','','最终240局完整性、CLI种子/胜者/轮数、回放嵌入地图/胜者、源码快照、冻结生产树及运行故障检查均通过。无动作死亡是比赛事件，不单独视为进程故障或排除依据。',
            '详细机制及局限见DIAGNOSIS.md；Default/Portals全部关键回放见REPLAYS.md；逐局数据和完整检查见final_verification.json。',
            '复现按PLAN.md命令，使用最终种子74001/74002/74003和新的输出目录；重跑相同种子不构成新的独立验证。']
    (ROOT/'REPORT.md').write_text('\n'.join(out)+'\n')
    replay = ['# Default / Portals 最终验证回放','','包括所有胜负，不选择性排除失败。','','| 对手 | 地图 | 种子 | counter方 | 结果 | 轮数 | 回放 |','|---|---|---:|---|---|---:|---|']
    for r in sorted(new['rows'],key=lambda r:(r['opponent'],r['map'],r['seed'],r['bot1_side'])):
        if r['map'] in ('default','portals'):
            replay.append(f"| {r['opponent']} | {r['map']} | {r['seed']} | {r['bot1_side']} | {r['outcome']} | {r['rounds']} | [{r['replay']}](counter_final/{r['replay']}) |")
    (ROOT/'REPLAYS.md').write_text('\n'.join(replay)+'\n')
    print(json.dumps({'success':success,'frozen_manifests_match':True,'final_games':240,'report':str(ROOT/'REPORT.md')},ensure_ascii=False))

if __name__ == '__main__': main()
