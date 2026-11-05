// main/sc_store.h —— 录音文件存储(LittleFS,分区 "storage")。
//
// 每条录音:
//   /rec/<id>.dat   96 字节头(magic、id、起止时间、帧数、文件密钥、nonce)+ 帧记录 [len u8][opus]
//   /rec/<id>.idx   每 SC_STORE_IDX_STRIDE 帧一个 u32 文件偏移,用于断点续传时定位
// 对外报告的文件大小 = 帧数 x 160(线上每帧固定 160 字节单元)。
// 所有函数线程安全(内部互斥);写入只来自录音任务,读取来自传输任务/HTTP 任务。
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "sdkconfig.h"

#include "esp_err.h"
#include "sc_proto.h"
#include "meeting_scr1_file.h"
#include "meeting_snapshot.h"
#include "meeting_receipt.h"
#include "meeting_cleanup.h"
#include "meeting_upload_protocol.h"
#include "meeting_delete_ui.h"

#define SC_STORE_MAX_FILES   200u
#define SC_STORE_IDX_STRIDE  256u
#define SC_STORE_MIN_FREE_KB 96u    // 低于该剩余空间拒绝开始/自动停止录音
#define SC_STORE_MAX_FRAMES 90000u  // 30 minutes at 50 Opus frames/second

typedef struct {
    uint32_t file_id;
    uint32_t start_time;
    uint32_t end_time;
    uint32_t frames;
    uint8_t file_key[32];
    uint8_t nonce[16];
    bool live;                  // 正在录制(或暂停中)
} sc_store_info_t;

esp_err_t sc_store_init(void);
/* False includes mount failure or an unsafe catalog; never implies empty. */
bool sc_store_ready(void);
#if CONFIG_MEETING_SUPERVISED_TEST_NEARFULL
/* Device-specific test fixture, never an existing recording. */
bool sc_store_test_nearfull_prepare(void);
bool sc_store_test_nearfull_cleanup(void);
#endif
/* A validated header-only interrupted recording remains present for review. */
bool sc_store_has_empty_interrupted(void);

// 文件清单,新的在前。返回条数。
size_t sc_store_list(sc_file_entry_t *out, size_t cap);
size_t sc_store_list_range(size_t start,sc_file_entry_t *out,size_t cap);
size_t sc_store_count(void);
bool sc_store_get(uint32_t file_id, sc_store_info_t *out);
void sc_store_usage(uint32_t *total_kb, uint32_t *free_kb);

// ---- 录制(仅录音任务调用)----
esp_err_t sc_store_begin(uint32_t file_id, uint32_t start_time);
esp_err_t sc_store_begin_timed(uint32_t file_id, uint32_t start_time, int64_t trusted_epoch);
bool sc_store_record_time(uint32_t file_id, bool *present, int64_t *epoch, uint8_t *trust);
esp_err_t sc_store_append(const uint8_t *frame, uint8_t len);
// 把已写入的帧落盘(暂停时调用,断电不丢暂停前的录音)。
esp_err_t sc_store_sync(void);
// 结束当前录音并落盘头部。
esp_err_t sc_store_end(uint32_t end_time, uint8_t end_intent);
uint32_t sc_store_live_id(void);       // 0 = 无
uint32_t sc_store_live_frames(void);

// ---- 读取 ----
// 一个读者顺序读一条录音;同一时刻最多 SC_STORE_READERS 个。
#define SC_STORE_READERS 2
typedef struct sc_reader sc_reader_t;

sc_reader_t *sc_store_reader_open(uint32_t file_id);
// 从 seq 起读最多 max 帧到 bufs(每帧 160 字节槽位)与 lens;返回实际帧数
// (0 = 已到当前末尾;<0 = 错误)。
int sc_store_reader_read(sc_reader_t *rd, uint32_t seq, uint8_t (*bufs)[SC_UNIT_DATA],
                         uint8_t *lens, int max);
void sc_store_reader_close(sc_reader_t *rd);

// ---- 管理 ----
esp_err_t sc_store_delete(uint32_t file_id);
esp_err_t sc_store_wipe(void);

// Read-only recovery inspection; does not alter the source or make it uploadable.
bool sc_store_inspect_recovery(uint32_t file_id, meeting_scr1_scan_result *out);

// Create or revalidate a recording snapshot without modifying audio.
esp_err_t sc_store_prepare_snapshot(uint32_t file_id, meeting_snapshot *out, meeting_cancel_fn cancelled, void *ctx);

// One pinned, read-only upload source. Caller must close using release, not fclose.
// Pin blocks new recording and deletion until the upload worker has exited.
FILE *sc_store_upload_acquire(uint32_t file_id);
esp_err_t sc_store_upload_release(FILE *file);
/* Begin a user-authorized deletion: serialize with upload/cleanup ownership,
 * persist an idempotent intent, and forbid subsequent upload. Does not delete. */
esp_err_t sc_store_begin_delete_checked(uint32_t file_id,meeting_delete_visibility reviewed,
                                       meeting_cancel_fn cancelled,void *ctx);
/* Task-only: may scan the full file. UNKNOWN includes active upload, errors,
 * corruption and uncertain identity; UNCONFIRMED is only local knowledge. */
meeting_delete_visibility sc_store_delete_visibility(uint32_t file_id,meeting_cancel_fn cancelled,void *ctx);
/* Only for an existing deletion intent. Pins the source, validates the
 * snapshot and persists the expected remote manifest; does not delete audio. */
esp_err_t sc_store_prepare_delete_proof(uint32_t file_id,meeting_cancel_fn cancelled,void *ctx);
/* Task-only; caller holds recording/upload arbitration. Idempotently removes
 * the local files only after verifying the durable intent and proof. */
esp_err_t sc_store_finish_delete_local(uint32_t file_id,meeting_cancel_fn cancelled,void *ctx);
/* Task-only, caller owns upload arbitration; unknown network outcome remains
 * pending. A committed response must match the saved expected manifest. */
esp_err_t sc_store_cancel_delete_remote(uint32_t file_id,const meeting_upload_transport *transport);
/* Uses durable receipt, snapshot and identity; never deletes on uncertainty.
 * Caller holds the recording/upload arbitration until this returns. */
meeting_cleanup_result sc_store_cleanup_confirmed(uint32_t file_id, meeting_cancel_fn cancelled, void *ctx);
/* Enumerates persisted cleanup intents; caller holds upload arbitration. */
meeting_cleanup_result sc_store_resume_cleanups(meeting_cancel_fn cancelled, void *ctx);
bool sc_store_cleanup_recovery_failed(void);
/* Only after a strictly verified receipt, while the matching source remains pinned. */
esp_err_t sc_store_confirm_upload(FILE *file, const meeting_snapshot *snapshot, const meeting_manifest *manifest);
typedef enum { SC_CONFIRMATION_ABSENT, SC_CONFIRMATION_PRESENT, SC_CONFIRMATION_ERROR } sc_confirmation_status;
/* Validates and re-commits an existing receipt against the pinned immutable
 * source. A malformed or conflicting record is an error, never absence. */
sc_confirmation_status sc_store_check_confirmation(FILE *file, const meeting_snapshot *snapshot,
                                                     bool has_time, int64_t recorded_at, uint8_t time_trust,
                                                     meeting_cancel_fn cancelled, void *ctx);
