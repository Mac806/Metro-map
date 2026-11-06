#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

#define MAX_STATIONS 1000 // 최대 역 개수
#define S_NAME_LEN 50 // 역 이름 최대 길이
#define LINE_NAME_LEN 20 // 호선 이름 최대 길이
#define TRAIN_SPEED 30 // 기차 속도 (km/h)
#define FILENAME_LEN 128 // CSV 파일 이름 최대 길이
#define MAX_LINE_LEN 256 // CSV 한 줄 최대 길이

// ---------- 구조체 ----------
typedef struct Edge {
	struct Station* destination; // 목적지 역
	int time; // 걸리는 시간 (분)
	float distance; // 거리 (km)
	char line[LINE_NAME_LEN]; // 호선 이름
	struct Edge* next; // 다음 엣지
} Edge;

typedef struct Station {
	char name[S_NAME_LEN]; // 역 이름
	int drugstoreCount; // 약국 수
	float restaurantRating; // 맛집 평균 평점
	Edge* edges; // 연결된 엣지 리스트
	struct Station* next; // 다음 역
} Station;

typedef struct Metro {
	Station* stations; // 역 리스트
} Metro;

typedef enum { // 가중치 타입
    WEIGHT_TIME = 0,
    WEIGHT_DISTANCE = 1,
    WEIGHT_DRUGSTORE = 2,
    WEIGHT_RESTAURANT = 3
} WeightType;

// ---------- 함수 ----------
float Distance(int time) { // 시간(분)을 거리(km)로 변환
    return (float)time * TRAIN_SPEED / 60.0;
}

Station* findStation(Metro* metro, const char* name) {
	Station* cur = metro->stations; // 역 리스트를 순회하며 이름이 일치하는 역 찾기
	while (cur) { // 역 이름 비교
		if (strcmp(cur->name, name) == 0) // 이름이 일치하면 해당 역 반환
            return cur; 
		cur = cur->next; // 다음 역으로 이동
    }
    return NULL;
}

void addstation(Metro* metro, const char* name) {
	if (findStation(metro, name)) return; // 이미 존재하는 역은 추가하지 않음
	Station* s = (Station*)malloc(sizeof(Station)); // 새 역 생성
	strcpy(s->name, name); // 역 이름 설정
	s->drugstoreCount = 0; // 약국 수 초기화
	s->restaurantRating = 0.0; // 맛집 평점 초기화
	s->edges = NULL; // 엣지 리스트 초기화
	s->next = NULL; // 다음 역 포인터 초기화
	if (!metro->stations) metro->stations = s; // 만약 역 리스트가 비어있다면 새 역을 첫 번째로 설정
	else { // 그렇지 않다면 리스트의 끝에 추가
		Station* cur = metro->stations; // 현재 역 리스트의 첫 번째 역부터 시작
		while (cur->next) cur = cur->next; // 리스트의 끝까지 이동
		cur->next = s; // 현재 역의 다음 포인터를 새 역으로 설정
    }
}

void addedge(Metro* metro, const char* from, const char* to, int time, const char* line) {
	Station* f = findStation(metro, from); // 출발 역 찾기
	Station* t = findStation(metro, to); // 도착 역 찾기
	if (!f || !t) return; // 출발 역이나 도착 역이 존재하지 않으면 함수 종료
	Edge* e = (Edge*)malloc(sizeof(Edge)); // 새 엣지 생성
	e->destination = t; // 목적지 역 설정
	e->time = time; // 걸리는 시간 설정
	e->distance = Distance(time); // 거리 계산
	strncpy(e->line, line, LINE_NAME_LEN - 1); // 호선 이름 설정
	e->line[LINE_NAME_LEN - 1] = '\0'; // 호선 이름 문자열 끝에 널 문자 추가
	e->next = f->edges; // 새 엣지를 현재 역의 엣지 리스트의 맨 앞에 추가
	f->edges = e; // 출발 역의 엣지 리스트를 새 엣지로 업데이트
}

// ---------- CSV 로딩 ----------
int loadCSV(Metro* metro, const char* filename) {
	FILE* file = fopen(filename, "r"); // CSV 파일 열기
	if (!file) return -1; // 파일 열기에 실패하면 -1 반환
	char line[MAX_LINE_LEN]; // 한 줄을 저장할 버퍼
	while (fgets(line, sizeof(line), file)) { // 파일에서 한 줄씩 읽기
		char from[S_NAME_LEN], to[S_NAME_LEN], lineName[LINE_NAME_LEN]; // 역 이름과 호선 이름을 저장할 버퍼
		int time, drugstore; // 시간(분)과 약국 수를 저장할 변수
		float rating; // 맛집 평균 평점을 저장할 변수
        // CSV 형식에 맞게 읽기 6은 성공적으로 읽은 항목의 개수
		if (sscanf(line, "%49[^,],%49[^,],%d,%d,%f,%19[^,]\n", from, to, &time, &drugstore, &rating, lineName) == 6) {          
			lineName[strcspn(lineName, "\n")] = 0; // 끝의 개행 제거 -> 출력때 문제 발생 방지
            addstation(metro, from); // 출발 역 추가
			Station* t = findStation(metro, to); // 도착 역 찾기
        if (!t) { // 만약 도착 역이 없다면 새로 추가
			addstation(metro, to); // 도착 역 추가
			t = findStation(metro, to); // 새로 추가한 도착 역 찾기
        }
		t->drugstoreCount = drugstore; // 도착 역의 약국 수 설정
		t->restaurantRating = rating; // 도착 역의 맛집 평균 평점 설정
		addedge(metro, from, to, time, lineName); // 엣지 추가
    }
}
fclose(file);
return 0;
}

// ---------- Merge Sort + 이진 탐색 ----------
int countStations(Metro* metro) { // 역의 개수를 세는 함수
	int c = 0; Station* s = metro->stations; // 역 리스트를 순회하며 개수 세기
	while (s) { c++; s = s->next; } // 역이 더 이상 없을 때까지 반복
    return c; 
}

void buildStationArray(Metro* metro, Station* arr[]) { // 역 리스트를 배열로 변환하는 함수
	int i = 0; Station* s = metro->stations; // 역 리스트를 순회하며 배열에 저장
	while (s) { arr[i++] = s; s = s->next; } // 역이 더 이상 없을 때까지 반복
}


// 병합 정렬을 이용해 역 이름 기준 오름차순 정렬
void merge(Station* arr[], int left, int mid, int right) { 
    int n1 = mid - left + 1;  // 왼쪽 배열의 크기 
    int n2 = right - mid;     // 오른쪽 배열의 크기

    // 동적 할당을 사용하여 배열 생성
    Station** L = (Station**)malloc(n1 * sizeof(Station*));
    Station** R = (Station**)malloc(n2 * sizeof(Station*));

    // 왼쪽 배열(L)과 오른쪽 배열(R)에 원소 복사
    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;

    // 두 배열을 병합하여 정렬된 상태로 arr에 저장
    while (i < n1 && j < n2) {
        if (strcmp(L[i]->name, R[j]->name) <= 0)
            arr[k++] = L[i++];
        else
            arr[k++] = R[j++];
    }

    // 왼쪽 배열(L) 또는 오른쪽 배열(R)에 남은 원소들 복사
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    // 동적 메모리 해제
    free(L);
    free(R);
}


void mergeSort(Station* arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

int findStationIndex(Station* arr[], int n, const char* name) {
	int l = 0, r = n - 1; // 이진 탐색을 위한 초기 인덱스 설정
	while (l <= r) {  // 이진 탐색 시작
		int m = (l + r) / 2; // 중간 인덱스 계산
		int cmp = strcmp(arr[m]->name, name); // 중간 역 이름과 찾고자 하는 역 이름 비교
		if (cmp == 0) return m; // 만약 중간 역 이름이 찾고자 하는 역 이름과 같다면 인덱스 반환
		else if (cmp < 0) l = m + 1; // 만약 중간 역 이름이 찾고자 하는 역 이름보다 작다면 왼쪽 범위를 좁힘
		else r = m - 1; // 만약 중간 역 이름이 찾고자 하는 역 이름보다 크다면 오른쪽 범위를 좁힘
    }
	return -1; // 찾고자 하는 역 이름이 없으면 -1 반환
}

float getEdgeWeight(Edge* edge, WeightType type) {
	Station* dest = edge->destination; // 목적지 역을 가져옴
    switch (type) { 
    case WEIGHT_TIME: return edge->time;
    case WEIGHT_DISTANCE: return edge->distance;
	case WEIGHT_DRUGSTORE: return -(float)dest->drugstoreCount; // 음수인 이유는 약국 수가 많을수록 더 좋은 경로로 간주하기 때문
	case WEIGHT_RESTAURANT: return -dest->restaurantRating; // 음수인 이유는 맛집 평점이 높을수록 더 좋은 경로로 간주하기 때문
 	default: return FLT_MAX; // 잘못된 타입일 경우 최대값 반환
    }
}

// ---------- 다익스트라 ----------
void dijkstra(Metro* metro, const char* startName, const char* endName, WeightType type) {
	int n = countStations(metro); // 역의 개수 세기
	Station* arr[MAX_STATIONS]; // 역을 저장할 배열
	buildStationArray(metro, arr); // 역 리스트를 배열로 변환
	mergeSort(arr, 0, n - 1); // 머지소트로 역 이름 기준으로 정렬
	int start = findStationIndex(arr, n, startName); // 시작 역 인덱스 찾기
	int end = findStationIndex(arr, n, endName); // 도착 역 인덱스 찾기
	if (start == -1 || end == -1) { printf("역을 찾을 수 없습니다.\n"); return; } // 시작 역이나 도착 역이 없으면 함수 종료

	float dist[MAX_STATIONS]; int visited[MAX_STATIONS], prev[MAX_STATIONS]; // 거리, 방문 여부, 이전 역을 저장할 배열
	char prev_line[MAX_STATIONS][LINE_NAME_LEN]; // 이전 역의 호선 이름을 저장할 배열
	for (int i = 0; i < n; i++) { // 모든 역 초기화
        dist[i] = FLT_MAX; visited[i] = 0; prev[i] = -1; prev_line[i][0] = '\0';
    }
	dist[start] = 0; // 시작 역의 거리는 0으로 설정

    for (int i = 0; i < n - 1; i++) {
		float min = FLT_MAX; int u = -1; // 최소 거리를 가진 역을 찾기 위한 변수
        for (int j = 0; j < n; j++) 
			if (!visited[j] && dist[j] < min) { min = dist[j]; u = j; } // 방문하지 않은 역 중에서 최소 거리를 가진 역 찾기
		if (u == -1) break; // 만약 모든 역이 방문되었거나 도달할 수 없는 경우 종료
		visited[u] = 1; // 현재 역을 방문 처리
		for (Edge* e = arr[u]->edges; e; e = e->next) { // 현재 역의 모든 엣지를 순회
			int v = findStationIndex(arr, n, e->destination->name); // 목적지 역의 인덱스 찾기
			if (v == -1 || visited[v]) continue; // 목적지 역이 없거나 이미 방문한 경우 건너뛰기
			float w = getEdgeWeight(e, type); // 엣지의 가중치 가져오기
			if (dist[u] + w < dist[v]) { // 만약 현재 역을 거쳐서 목적지 역으로 가는 거리가 더 짧다면
				dist[v] = dist[u] + w; // 거리 업데이트
				prev[v] = u; // 이전 역 업데이트
				strcpy(prev_line[v], e->line); // 이전 역의 호선 이름 업데이트
            }
        }
    }

	if (dist[end] == FLT_MAX) { printf("경로를 찾을 수 없습니다.\n"); return; } // 도착 역까지의 거리가 최대값이면 경로를 찾을 수 없다는 메시지 출력
     
	int path[MAX_STATIONS], cnt = 0, at = end; // 최단 경로를 저장할 배열과 카운터, 현재 역 인덱스
	while (at != -1) { path[cnt++] = at; at = prev[at]; } // 현재 역에서 이전 역으로 거슬러 올라가며 경로 저장  

    printf("최단 경로 (총 ");
    switch (type) {
    case WEIGHT_TIME: printf("시간(분): %.2f", dist[end]); break; 
    case WEIGHT_DISTANCE: printf("거리(km): %.2f", dist[end]); break;
    case WEIGHT_DRUGSTORE: printf("약국 기준값: %.0f", -dist[end]); break;
    case WEIGHT_RESTAURANT: printf("맛집 평점 기준값: %.2f", -dist[end]); break;
    }
    printf(")\n------------------------------------------------------\n");

    char last_line[LINE_NAME_LEN] = ""; 
	for (int i = cnt - 1; i >= 0; i--) { // 역을 거꾸로 출력
		Station* s = arr[path[i]]; // 현재 역
		const char* line = (i == cnt - 1) ? "" : prev_line[path[i]]; // 이전 역의 호선 이름
		if (i == cnt - 1) { // 첫 번째 역은 출발 역으로 표시
			printf("%s", s->name); // 출발 역 이름 출력
            if (type == WEIGHT_DRUGSTORE) printf("(출발, 약국 수: %d)", s->drugstoreCount); // 약국 기준
            else if (type == WEIGHT_RESTAURANT) printf("(출발, 평점: %.1f)", s->restaurantRating); // 출발역 기준의 맛집 평점 출력
        }
        else {
            if (strcmp(last_line, line) != 0 && last_line[0] != '\0')
                printf(" (환승: %s)", line);
            printf(" -> %s", s->name); // 출발역 이외의 역들은 화살표 붙음
			if (type == WEIGHT_TIME || type == WEIGHT_DISTANCE) printf("(%s)", line); // 시간과 거리 기준일 때 호선 이름 출력
			else if (type == WEIGHT_DRUGSTORE) printf("(%s, 약국 수: %d)", line, s->drugstoreCount); // 약국 기준일 때 호선 이름과 약국 수 출력
			else if (type == WEIGHT_RESTAURANT) printf("(%s, 평점: %.1f)", line, s->restaurantRating); // 맛집 평점 기준일 때 호선 이름과 평점 출력
        }
        strcpy(last_line, line);
    }
    printf("\n");
}

// ---------- CSV 직접 추가 ----------
void addEntryToCSV(const char* filename) {
    char from[S_NAME_LEN], to[S_NAME_LEN], line[LINE_NAME_LEN];
    int time, drugstore; float rating;
	printf("출발역 입력: "); 
    fgets(from, sizeof(from), stdin); 
    from[strcspn(from, "\n")] = 0;
    printf("도착역 입력: "); 
    fgets(to, sizeof(to), stdin); 
    to[strcspn(to, "\n")] = 0; 
    printf("걸리는 시간(분) 입력: "); 
    scanf("%d", &time); 
    getchar();
    printf("도착역 기준 약국 수 입력: "); 
    scanf("%d", &drugstore); 
    getchar();
    printf("도착역 기준 맛집 평균 평점 입력: "); 
    scanf("%f", &rating); 
    getchar();
    printf("호선 입력: "); 
    fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
    FILE* f = fopen(filename, "a");
    if (!f) { printf("파일 열기 실패\n"); return; }
    fprintf(f, "%s,%s,%d,%d,%.1f,%s\n", from, to, time, drugstore, rating, line);
    fclose(f);
    printf("CSV에 새로운 노선이 추가되었습니다.\n");
}

// ---------- 인터페이스 ----------
void show_menu() {
    printf("\n==========================\n");
    printf("1. CSV 새로고침\n");
    printf("2. 길찾기\n");
    printf("3. CSV 파일에 노선 추가\n");
    printf("0. 종료\n");
    printf("메뉴 선택: ");
}

void interface() {
    Metro metro = { NULL };
    char file[FILENAME_LEN] = "subway.csv";
    int loaded = 0;
    while (1) {
        show_menu();
        int menu; scanf("%d", &menu); 
        getchar();
        if (menu == 1) {
            printf("파일명 입력 (기본 subway.csv): ");
            char input[FILENAME_LEN];
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;
            if (strlen(input) > 0) {
                strcpy(file, input);
            }
            if (loadCSV(&metro, file) == 0) {
                loaded = 1;
                printf("로드 완료\n");
            }
            else printf("로드 실패\n");
        }
        else if (menu == 2) {
			if (!loaded) { 
                printf("먼저 CSV를 로드하세요\n"); 
                continue; 
            } 
            char start[S_NAME_LEN], end[S_NAME_LEN]; 
            int w;
            printf("출발역: "); 
            fgets(start, sizeof(start), stdin); 
            start[strcspn(start, "\n")] = 0;
            printf("도착역: "); 
            fgets(end, sizeof(end), stdin); 
            end[strcspn(end, "\n")] = 0;
            printf("가중치 선택 (0:시간, 1:거리, 2:약국, 3:맛집): ");
            scanf("%d", &w); 
            getchar();
            dijkstra(&metro, start, end, (WeightType)w);
        }
        else if (menu == 3) {
            addEntryToCSV(file);
        }
        else if (menu == 0) {
            printf("종료합니다.\n"); break;
        }
        else {
            printf("잘못된 입력\n");
        }
    }
}

int main() {
    interface();
    return 0;
}
