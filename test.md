# Project Plan: AirDroid Parental Control Clone

## 1. Project Overview

A production-ready parental control system. The solution consists of a React-based Parent Dashboard, a Kotlin-based Child Android App, and a Node.js/Express Signaling Server, utilizing Supabase for database and authentication.

## 2. Technology Stack & Free Tier Solutions

| Domain                    | Technology / Library        | Purpose                                             |
| ------------------------- | --------------------------- | --------------------------------------------------- |
| **Frontend (Parent)**     | React + Vite, TailwindCSS   | High-performance dashboard dashboard                |
| **State Management**      | Zustand                     | Lightweight, scalable state management              |
| **Mobile (Child)**        | Kotlin, Android SDK         | Native performance, deep OS access                  |
| **Backend / Signaling**   | Node.js, Express, Socket.io | Real-time command pushing, WebRTC signaling         |
| **Database & Auth**       | Supabase (PostgreSQL)       | Scalable RDBMS, Auth, Storage (Free tier optimized) |
| **Offline Storage**       | Kotlin Room DB              | Local data persistence when offline                 |
| **Real-time Video/Audio** | WebRTC                      | P2P streaming (Zero server bandwidth cost)          |

## 3. Feature Implementation Strategy

### 3.1 Device Data & Status Tracker

- **Battery, Online Status, Basic Data:**
  - **Tech:** Android BatteryManager + Socket.io / Supabase Realtime.
  - **Flow:** Child app sends periodic heartbeats.

### 3.2 Location Engine (Live & History)

- **Live Location:** Google Play Services `FusedLocationProviderClient` + Socket.io for real-time map updates on the parent dashboard.
- **Offline Location History:**
  - **Tech:** Room DB + Android WorkManager.
  - **Flow:** When offline, location coordinates append to Room DB. WorkManager observes network state; upon reconnect, syncs data to Supabase and clears local DB.

### 3.3 Media Streaming (Screen, Camera, Audio)

- **Tech:** WebRTC + Socket.io (for SDP exchange).
- **Flow:** Parent requests stream -> Node.js sends command via Socket.io -> Child accepts command -> WebRTC P2P connection established.
- **Simultaneous View:** WebRTC allows multi-track streams (Video track for Screen/Camera + Audio track for Mic).

### 3.4 Automated Recording & Snapshots

- **Screen Recording/Snapshot:** Android `MediaProjection` API.
- **Voice Recording:** `AudioRecord` or `MediaRecorder`.
- **Scheduling:** `AlarmManager` or `WorkManager` for scheduled captures.
- **Storage:** Captured media uploads directly to **Supabase Storage**. Supabase returns a public/signed URL saved to the PostgreSQL database.

### 3.5 OS Level Event Tracking (SMS, Calls, App Usage)

- **App Usage Tracking:** `UsageStatsManager` API (Requires usage access permission).
- **SMS & Call Logs:** `ContentObserver` for monitoring changes in SMS and Call Log content URIs.
- **Persistence:** `AccessibilityService` and `DeviceAdminReceiver` ensure the app remains active in the background and cannot be easily uninstalled by the child.

---

## 4. Architecture & Scalability

- **P2P Priority:** All heavy data (live screen, camera, audio) MUST route through WebRTC. The Node.js server only acts as a signaling server (exchanging connection info). This keeps server costs at absolute zero.
- **Database Indexing:** Supabase PostgreSQL tables must have indexes on `device_id` and `timestamp` for fast querying of historical data.

---

## 5. Strict AI Coding Standard & Anti-Hallucination Rules

To maintain consistency and prevent AI hallucination during the 10%-100% development phase, the AI must strictly adhere to the following rules:

### 5.1 No Boilerplate, No Placeholders

- Never generate code with comments like `// Add logic here`. Provide complete, working logic for the requested component.
- If a library requires an API key, use environment variables (`process.env` or `BuildConfig`).

### 5.2 Strict Typing

- **React/Node.js:** Use TypeScript strictly. Define interfaces for all API responses and database models.
- **Kotlin:** Use Data Classes. No `Any` or `dynamic` types.

### 5.3 Feature-Driven Development (FDD) Workflow

- Build one complete vertical slice at a time (e.g., complete "Live Location" from Kotlin DB -> Node.js -> Supabase -> React UI before touching "Screen Recording").

### 5.4 Standard AI Prompt Template for Future Features

When requesting the AI to build a new feature, use this exact prompt structure:

> "System: Read `plan.md` and `types.ts`/`Models.kt` first.
> Task: Implement [Feature Name].
> Rule: Use the existing architecture. Do not introduce new libraries unless strictly necessary. Provide the Kotlin implementation first, then the Node.js signaling (if needed), and finally the React UI."

### 5.5 Handling Deprecations

- The AI MUST check for deprecated Android APIs (e.g., `startActivityForResult` vs `ActivityResultContracts`) and use the modern modern AndroidX standard.
