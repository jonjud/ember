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
| Last code commit | [`60e05ca`](https://github.com/jonjud/ember/commit/60e05ca08af44ec305988ff9dd88ccfee1d5589e) (component schema migration) |
| Last verified CI | [Windows Build #92](https://github.com/jonjud/ember/actions/runs/45921944622) |
| CI result | **Success** — Build และ CTest ผ่าน |
| Last completed milestone | Versioned Component Schema Migration |
| Current active milestone | Collision Detection & Physics — Stage 1: backend dependency and architecture |
| Physics backend | Box2D v3.2.0 pinned via FetchContent; integration wrapper not implemented yet |
| Next planned milestone | Collision Detection and Physics Integration (Box2D evaluation/integration) |

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
| 2 | 2D Core and Project Assets | Core เสร็จ; Collision/Physics ยังไม่เริ่ม |
| 3 | Editor | ฟังก์ชันหลักมีแล้ว; Undo/Redo, Multi-selection และ interactive verification ยังเหลือ |
| 4 | Game Systems | Component system เสร็จ; Physics, Animation, Audio, UI, Runtime InputMap และ Scripting ยังเหลือ |
| 5 | Packaging and Quality | CI tests มีแล้ว; Export, Templates, Profiler และ User Docs ยังเหลือ |

## 4. งานถัดไปที่ตกลงไว้

### Active milestone: Collision Detection and Physics Integration

**Stage 1 — backend and architecture:** เลือก Box2D v3.2.0, เพิ่ม dependency แบบ pin version และกำหนดขอบเขต interface แล้ว; ยังไม่ถือว่ามี physics simulation ใช้งานได้จนกว่าจะมี wrapper และ tests

**เป้าหมาย:** วางระบบ Collision/Physics ให้ใช้งานร่วมกับ Entity/Component/Scene และ Play Mode ได้ โดยไม่ผูก physics dependency เข้ากับ Editor UI

ลำดับงานที่เสนอ:
1. [x] ตรวจ dependency และ pin Box2D v3.2.0; แยก Box2D ไว้หลัง engine-facing interface
2. [x] กำหนดแนวทาง PhysicsWorld/Scene ownership, coordinate conversion และ body/collider components ใน [PHYSICS_DESIGN.md](PHYSICS_DESIGN.md)
3. [ ] สร้าง PhysicsWorld wrapper และทดสอบ body/shape lifecycle
4. [ ] เชื่อม PhysicsWorld กับ Scene Start/Update/Stop และ fixed timestep
5. [ ] เพิ่ม PhysicsBody2D/Collider2D serialization และ Inspector properties
6. [ ] เพิ่ม automated tests สำหรับ collision, body lifecycle และ scene round-trip
7. [ ] เชื่อม debug visualization ใน Editor เมื่อ core ทำงานเสถียรแล้ว

**ยังไม่เริ่ม milestone นี้** จนกว่าจะมี commit แรกที่ลงมือทำจริง และจะไม่ติ๊กว่างานเสร็จจากการวางแผนเพียงอย่างเดียว

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
| 2026-11-27 | pending | Pin Box2D v3.2.0 and document Physics integration boundary | CI pending |

---

**วิธีใช้:** เปิดไฟล์นี้ก่อนเริ่มทำงานทุกครั้ง และแก้ไขไฟล์นี้พร้อม Roadmap เมื่อสถานะเปลี่ยน เพื่อให้เอกสารสะท้อนสถานะจริงของ repository ไม่ใช่ความทรงจำจากบทสนทนา
