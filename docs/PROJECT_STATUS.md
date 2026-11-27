# EMBER Engine — Living Project Status

> **เอกสารสถานะหลักของโปรเจกต์ (Single Source of Truth)**  
> อัปเดตทุกครั้งที่จบหรือเปลี่ยน milestone เพื่อให้การพัฒนาต่อเนื่องข้ามบทสนทนาได้โดยไม่หลุดเป้าหมาย  
> Roadmap ภาพรวมอยู่ที่ [ROADMAP.md](ROADMAP.md) และรายละเอียดโครงสร้างอยู่ที่ [ARCHITECTURE.md](ARCHITECTURE.md)

## 1. สถานะปัจจุบัน

| รายการ | ค่า |
|---|---|
| Project | EMBER Engine |
| Repository | [jonjud/ember](https://github.com/jonjud/ember) |
| Active branch | `develop` |
| Stable branch | `main` |
| Last code commit | [`b1d7902`](https://github.com/jonjud/ember/commit/b1d79028c1f8184a5543d77ca339d8acd269e8d4) (Box2D dependency integration) |
| Last verified code CI | [Windows Build #95](https://github.com/jonjud/ember/actions/runs/45922737396) — **Success**; Box2D dependency commit `b1d7902` |
| Latest CI | [Windows Build #96](https://github.com/jonjud/ember/actions/runs/45922782711) — **Success**; status-document update commit `a78e461` |
| Last completed milestone | Versioned Component Schema Migration |
| Current active milestone | Collision Detection & Physics — Stage 2: PhysicsWorld wrapper and tests |
| Physics backend | Box2D v3.2.0 pinned via FetchContent; integration wrapper not implemented yet |
| Next planned milestone | Scene lifecycle integration and fixed timestep |

> CI Success หมายถึงการ Compile และ Automated Tests ผ่านใน Windows workflow เท่านั้น ไม่ได้ยืนยันพฤติกรรม Editor/Runtime แบบ interactive บนเครื่องจริง

## 2. สิ่งที่เพิ่งเสร็จ

### Component System — complete

- [x] Component lifecycle: create, update, destroy, clone
- [x] Stable Component Registry และ Factory
- [x] Generic component serialization/deserialization
- [x] แสดง Component ใน Inspector
- [x] เพิ่มและลบ registered Component ผ่าน Editor UI
- [x] Typed Property Descriptors และ generic Inspector widgets: bool, int, float, string
- [x] Save/Load ค่าของ generic Component
- [x] Component schema version ต่อชนิด
- [x] Sequential migration callbacks สำหรับข้อมูล Component รุ่นเก่า
- [x] รองรับไฟล์ Component รุ่นก่อนที่ไม่มี version โดยตีความเป็น version 1
- [x] ปฏิเสธ future schema version หรือ migration ที่ขาด/ล้มเหลวอย่างปลอดภัย
- [x] Unit tests สำหรับ registry, migration และ scene serialization

**ข้อจำกัดที่ต้องจำ:** เพิ่ม/ลบ Component ที่ลงทะเบียนไว้ได้จาก UI แต่การสร้าง Component type หรือพฤติกรรมใหม่ยังต้องเขียน C++ และ register type/property/serialization callbacks ก่อน

## 3. ภาพรวม Roadmap

สถานะนี้เป็นภาพย่อ ไม่แทนรายการย่อยทั้งหมดใน [ROADMAP.md](ROADMAP.md)

| Phase | หัวข้อ | สถานะ |
|---|---|---|
| 1 | Engine Foundation | Core เสร็จ; ยังมี runtime verification และ logging |
| 2 | 2D Core and Project Assets | Core เสร็จ; Box2D backend pinned; simulation ยังไม่เริ่ม |
| 3 | Editor | ฟังก์ชันหลักมีแล้ว; Undo/Redo, Multi-selection และ interactive verification ยังเหลือ |
| 4 | Game Systems | Component system เสร็จ; Physics integration, Animation, Audio, UI, Runtime InputMap และ Scripting ยังเหลือ |
| 5 | Packaging and Quality | CI tests มีแล้ว; Export, Templates, Profiler และ User Docs ยังเหลือ |

## 4. งานถัดไปที่ตกลงไว้

### Active milestone: Collision Detection and Physics Integration

**Stage 1 — backend and architecture: complete.** Pin Box2D v3.2.0 และกำหนดขอบเขต interface แล้ว

**Stage 2 — PhysicsWorld wrapper and tests: active.** สร้าง RAII wrapper และ engine-owned body/shape API พร้อม automated tests; ยังไม่ถือว่ามี Scene/Editor integration จนกว่าจะทำขั้นถัดไป

**เป้าหมาย:** วางระบบ Collision/Physics ให้ใช้งานร่วมกับ Entity/Component/Scene และ Play Mode ได้ โดยไม่ผูก physics dependency เข้ากับ Editor UI

ลำดับงานที่เสนอ:
1. [x] ตรวจ dependency และ pin Box2D v3.2.0; แยก Box2D ไว้หลัง engine-facing interface
2. [x] กำหนดแนวทาง PhysicsWorld/Scene ownership, coordinate conversion และ body/collider components ใน [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)
3. [ ] สร้าง PhysicsWorld wrapper และทดสอบ body/shape lifecycle
4. [ ] เชื่อม PhysicsWorld กับ Scene Start/Update/Stop และ fixed timestep
5. [ ] เพิ่ม PhysicsBody2D/Collider2D serialization และ Inspector properties
6. [ ] เพิ่ม automated tests สำหรับ collision, body lifecycle และ scene round-trip
7. [ ] เชื่อม debug visualization ใน Editor เมื่อ core ทำงานเสถียรแล้ว

**ขอบเขตปัจจุบัน:** backend และ design ลง repository แล้ว แต่ยังไม่มี simulation wrapper หรือ collision tests จึงยังไม่ถือว่า Physics integration เสร็จ

## 5. กติกาการพัฒนาและการอัปเดตเอกสาร

1. ทำงานบน `develop` เท่านั้น เว้นแต่ผู้ใช้อนุมัติอย่างชัดเจนให้เปลี่ยน branch
2. ห้าม merge เข้า `main` หากยังไม่ได้รับอนุมัติ
3. ก่อนแก้โค้ดทุกครั้ง ต้องตรวจ HEAD ล่าสุดของ `develop` ก่อน
4. ทำเป็น milestone เล็ก ๆ ที่ build และ test ได้
5. เมื่อจบ milestone ให้:
   - อัปเดต checkbox ใน `ROADMAP.md` ตามสิ่งที่ทำจริง
   - อัปเดตไฟล์นี้: last completed, active/next milestone, commit, CI status และ known gaps
   - อัปเดต `ARCHITECTURE.md` หากโครงสร้างหรือพฤติกรรมระบบเปลี่ยน
6. ห้ามรายงาน CI ว่าผ่านจนกว่า GitHub Actions จะแสดง `completed` และ `conclusion: success`
7. แยกสถานะ Build/Automated Tests ออกจากการทดสอบ Editor/Runtime แบบ interactive เสมอ
8. หาก CI Failed ให้บันทึกสาเหตุ แก้ไข และรอผล CI รอบใหม่ก่อนปิด milestone
9. อย่าขยาย scope ออกจากเป้าหมายที่ตกลงกันโดยไม่แจ้งผู้ใช้

## 6. บันทึกการเปลี่ยนแปลงล่าสุด

| วันที่ | Commit | รายละเอียด | CI |
|---|---|---|---|
| 2026-11-27 | [`60e05ca`](https://github.com/jonjud/ember/commit/60e05ca08af44ec305988ff9dd88ccfee1d5589e) | Versioned component schema migrations; legacy v1 compatibility; migration tests | [Build #92 — Success](https://github.com/jonjud/ember/actions/runs/45921944622) |
| 2026-11-27 | [`b1d7902`](https://github.com/jonjud/ember/commit/b1d79028c1f8184a5543d77ca339d8acd269e8d4) | Pin Box2D v3.2.0, enable C language, document Physics boundary | [Build #95 — Success](https://github.com/jonjud/ember/actions/runs/45922737396) |
| 2026-11-27 | [`a78e461`](https://github.com/jonjud/ember/commit/a78e4618f3483c3d3d6be987b704fb9ea466a62d) | Update Physics milestone tracking and design docs | [Build #96 — Success](https://github.com/jonjud/ember/actions/runs/45922782711) |

---

**วิธีใช้:** เปิดไฟล์นี้ก่อนเริ่มทำงานทุกครั้ง และแก้ไขไฟล์นี้พร้อม Roadmap เมื่อสถานะเปลี่ยน เพื่อให้เอกสารสะท้อนสถานะจริงของ repository ไม่ใช่ความทรงจำจากบทสนทนา
