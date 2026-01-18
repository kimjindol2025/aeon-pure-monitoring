#!/usr/bin/env python3
"""
AI Gateway 198 모니터링 데이터 수집기
철학: 순수 구현 (외부 라이브러리 최소화)
"""

import sqlite3
import subprocess
import json
import time
from datetime import datetime

DB_PATH = '../data/aeon_evolution.db'
SSH_CMD = 'ssh -p 22 kim@192.168.45.198'

def init_gateway_table():
    """Gateway 통계 테이블 생성"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute('''
        CREATE TABLE IF NOT EXISTS gateway_stats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            server TEXT NOT NULL,
            pid INTEGER,
            phase TEXT,
            status TEXT,
            uptime_days REAL,
            total_commands INTEGER,
            success_rate REAL,
            block_rate REAL,
            memory_usage_mb INTEGER,
            level TEXT
        )
    ''')

    conn.commit()
    conn.close()
    print('[Gateway Collector] 테이블 초기화 완료')

def fetch_gateway_status():
    """198 서버에서 Gateway 상태 가져오기"""
    try:
        # AI_STATUS_SUMMARY.json 가져오기
        cmd = f"{SSH_CMD} 'cat /home/kim/ai-gateway-198/AI_STATUS_SUMMARY.json'"
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=5)

        if result.returncode != 0:
            print(f'[Gateway Collector] SSH 실패: {result.stderr}')
            return None

        data = json.loads(result.stdout)

        # heartbeat.json에서 PID 가져오기
        cmd_hb = f"{SSH_CMD} 'cat /home/kim/ai-gateway-198/state/heartbeat.json'"
        result_hb = subprocess.run(cmd_hb, shell=True, capture_output=True, text=True, timeout=5)

        pid = 0
        if result_hb.returncode == 0:
            try:
                hb_data = json.loads(result_hb.stdout)
                pid = hb_data.get('pid', 0)
            except:
                pass

        # 데이터 추출
        stats = {
            'timestamp': int(time.time()),
            'server': data.get('server', '192.168.45.198'),
            'pid': pid,
            'phase': data.get('learning_summary', {}).get('phase', 'Unknown'),
            'status': data.get('learning_summary', {}).get('status', 'Unknown'),
            'uptime_days': data.get('learning_summary', {}).get('uptime_days', 0),
            'total_commands': data.get('learning_summary', {}).get('total_commands', 0),
            'success_rate': data.get('learning_summary', {}).get('success_rate', 0),
            'block_rate': data.get('learning_summary', {}).get('block_rate', 0),
            'memory_usage_mb': data.get('performance_metrics', {}).get('memory_usage_mb', 0),
            'level': data.get('security_learning', {}).get('level', 'Unknown')
        }

        return stats

    except subprocess.TimeoutExpired:
        print('[Gateway Collector] SSH 타임아웃')
        return None
    except json.JSONDecodeError as e:
        print(f'[Gateway Collector] JSON 파싱 실패: {e}')
        return None
    except Exception as e:
        print(f'[Gateway Collector] 오류: {e}')
        return None

def log_gateway_stats(stats):
    """DB에 Gateway 상태 기록"""
    if not stats:
        return False

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()

        cursor.execute('''
            INSERT INTO gateway_stats
            (timestamp, server, pid, phase, status, uptime_days,
             total_commands, success_rate, block_rate, memory_usage_mb, level)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            stats['timestamp'],
            stats['server'],
            stats['pid'],
            stats['phase'],
            stats['status'],
            stats['uptime_days'],
            stats['total_commands'],
            stats['success_rate'],
            stats['block_rate'],
            stats['memory_usage_mb'],
            stats['level']
        ))

        conn.commit()
        conn.close()
        return True

    except Exception as e:
        print(f'[Gateway Collector] DB 기록 실패: {e}')
        return False

def print_gateway_stats(stats):
    """Gateway 상태 출력"""
    if not stats:
        return

    print(f"[Gateway Stats] Server:{stats['server']} | PID:{stats['pid']} | "
          f"Phase:{stats['phase']} | Status:{stats['status']}")
    print(f"                Uptime:{stats['uptime_days']:.2f} days | "
          f"Commands:{stats['total_commands']} | "
          f"Success:{stats['success_rate']:.1f}% | "
          f"Block:{stats['block_rate']:.1f}%")
    print(f"                Memory:{stats['memory_usage_mb']}MB | "
          f"Level:{stats['level']}")

def main():
    """메인 루프: 5초마다 상태 수집"""
    print('[Gateway Collector] 시작 - 5초마다 198 서버 모니터링')
    print('[Ctrl+C로 중지]\n')

    # 테이블 초기화
    init_gateway_table()

    try:
        while True:
            stats = fetch_gateway_status()

            if stats:
                print_gateway_stats(stats)
                log_gateway_stats(stats)
            else:
                print('[Gateway Collector] 데이터 수집 실패')

            time.sleep(5)

    except KeyboardInterrupt:
        print('\n[Gateway Collector] 중지됨')

if __name__ == '__main__':
    main()
