import React, { useState, useEffect } from 'react';
import { 
  Trophy, Code2, Play, RefreshCw, UserCheck, CheckCircle2, 
  XCircle, Clock, Database, Sparkles, LogIn, UserPlus, 
  LogOut, Search, Eye, Copy, Check, Terminal, FileCode, AlertCircle, ShieldAlert
} from 'lucide-react';
import AdminPanel from './AdminPanel';

const API_BASE = 'http://localhost:8080/api';

const PROBLEM_TEMPLATES = {
  1: `// Problem 1: Two Sum
#include <iostream>
using namespace std;

int main() {
    long long a, b;
    if (cin >> a >> b) {
        cout << (a + b) << endl;
    }
    return 0;
}`,
  2: `// Problem 2: Palindrome Check
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

int main() {
    string s;
    if (cin >> s) {
        string rev = s;
        reverse(rev.begin(), rev.end());
        if (s == rev) cout << "YES" << endl;
        else cout << "NO" << endl;
    }
    return 0;
}`,
  3: `// Problem 3: N-th Fibonacci Number
#include <iostream>
using namespace std;

int main() {
    int n;
    if (cin >> n) {
        if (n == 0) { cout << 0 << endl; return 0; }
        if (n == 1) { cout << 1 << endl; return 0; }
        long long a = 0, b = 1;
        for (int i = 2; i <= n; ++i) {
            long long c = a + b;
            a = b;
            b = c;
        }
        cout << b << endl;
    }
    return 0;
}`,
  4: `// Problem 4: Array Min-Max and Sum
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    int n;
    if (cin >> n && n > 0) {
        long long mn = 2e18, mx = -2e18, sum = 0;
        for (int i = 0; i < n; ++i) {
            long long val;
            cin >> val;
            mn = min(mn, val);
            mx = max(mx, val);
            sum += val;
        }
        cout << mn << " " << mx << " " << sum << endl;
    }
    return 0;
}`,
  5: `// Problem 5: Prime Counter (Sieve of Eratosthenes)
#include <iostream>
#include <vector>
using namespace std;

int main() {
    int n;
    if (cin >> n) {
        if (n < 2) { cout << 0 << endl; return 0; }
        vector<bool> is_prime(n + 1, true);
        is_prime[0] = is_prime[1] = false;
        for (int p = 2; p * p <= n; ++p) {
            if (is_prime[p]) {
                for (int i = p * p; i <= n; i += p)
                    is_prime[i] = false;
            }
        }
        int count = 0;
        for (int i = 2; i <= n; ++i) {
            if (is_prime[i]) count++;
        }
        cout << count << endl;
    }
    return 0;
}`,
  6: `// Problem 6: Balanced Parentheses
#include <iostream>
#include <string>
#include <stack>
using namespace std;

int main() {
    string s;
    if (cin >> s) {
        stack<char> st;
        bool ok = true;
        for (char c : s) {
            if (c == '(' || c == '[' || c == '{') {
                st.push(c);
            } else if (c == ')' || c == ']' || c == '}') {
                if (st.empty()) { ok = false; break; }
                char top = st.top();
                st.pop();
                if ((c == ')' && top != '(') ||
                    (c == ']' && top != '[') ||
                    (c == '}' && top != '{')) {
                    ok = false;
                    break;
                }
            }
        }
        if (ok && st.empty()) cout << "VALID" << endl;
        else cout << "INVALID" << endl;
    }
    return 0;
}`,
  7: `// Problem 7: FCFS CPU Scheduling Simulation
#include <iostream>
#include <vector>
using namespace std;

int main() {
    int n;
    if (cin >> n && n > 0) {
        long long current_time = 0;
        long long total_turnaround = 0;
        for (int i = 0; i < n; ++i) {
            long long burst;
            cin >> burst;
            current_time += burst;
            total_turnaround += current_time;
        }
        cout << total_turnaround << endl;
    }
    return 0;
}`
};

const COMMON_ERROR_TEMPLATES = {
  wa: `// Logic Error Example (Wrong Answer)
#include <iostream>
using namespace std;

int main() {
    long long a;
    if (cin >> a) {
        // Deliberately prints incorrect answer to demonstrate WA verdict
        cout << (a * 99999 + 42) << endl;
    }
    return 0;
}`,
  tle: `// Infinite Loop Example (Time Limit Exceeded)
#include <iostream>
using namespace std;

int main() {
    volatile int x = 0;
    while (true) {
        x++; // Runs endlessly until sandbox timeout triggers (3.0 seconds)
    }
    return 0;
}`,
  mle: `// Memory Exhaustion Example (Memory Limit Exceeded)
#include <iostream>
#include <vector>
using namespace std;

int main() {
    // Attempts to allocate 200MB, exceeding the 64MB memory quota
    vector<int> huge(50000000, 1);
    cout << huge[0] << endl;
    return 0;
}`
};

export default function App() {
  const [activeTab, setActiveTab] = useState('home'); // home | editor | leaderboard | submissions | race
  const [serverOnline, setServerOnline] = useState(false);
  
  // Auth state
  const [currentUser, setCurrentUser] = useState(() => localStorage.getItem('contest_user') || 'student_1');
  const [currentUserRole, setCurrentUserRole] = useState(() => localStorage.getItem('contest_role') || 'participant');
  const [sessionToken, setSessionToken] = useState(() => localStorage.getItem('contest_session_token') || '');
  const [adminToken, setAdminToken] = useState(() => localStorage.getItem('contest_admin_token') || '');
  const [inAdminMode, setInAdminMode] = useState(false);
  const [isAuthModalOpen, setIsAuthModalOpen] = useState(false);
  const [authMode, setAuthMode] = useState('login'); // login | register
  const [authUsername, setAuthUsername] = useState('');
  const [authPassword, setAuthPassword] = useState('');
  const [authError, setAuthError] = useState('');

  // Contest & Problems State
  const [problems, setProblems] = useState([]);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedProblem, setSelectedProblem] = useState(null);
  const [code, setCode] = useState(PROBLEM_TEMPLATES[1]);
  const [customInput, setCustomInput] = useState('');
  const [useCustomInput, setUseCustomInput] = useState(false);
  
  // Running & Submission State
  const [isRunning, setIsRunning] = useState(false);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [runResult, setRunResult] = useState(null);
  const [submissionResult, setSubmissionResult] = useState(null);
  const [notification, setNotification] = useState(null);

  // Leaderboard & Submissions History
  const [leaderboard, setLeaderboard] = useState([]);
  const [lbSearch, setLbSearch] = useState('');
  const [submissions, setSubmissions] = useState([]);
  const [viewingSubmission, setViewingSubmission] = useState(null);
  const [copied, setCopied] = useState(false);


  // Load Initial Data & Polling
  useEffect(() => {
    fetchProblems();
    fetchLeaderboard();
    fetchSubmissions();

    const interval = setInterval(() => {
      fetchLeaderboard();
      fetchSubmissions();
    }, 3000);

    return () => clearInterval(interval);
  }, []);

  const showToast = (message, type = 'success') => {
    setNotification({ message, type });
    setTimeout(() => setNotification(null), 4000);
  };

  const fetchProblems = async () => {
    try {
      const res = await fetch(`${API_BASE}/problems`);
      if (res.ok) {
        const data = await res.json();
        setProblems(data);
        if (data.length > 0 && !selectedProblem) {
          setSelectedProblem(data[0]);
          setCode(PROBLEM_TEMPLATES[data[0].id] || PROBLEM_TEMPLATES[1]);
        }
        setServerOnline(true);
      }
    } catch (e) {
      setServerOnline(false);
    }
  };

  const fetchLeaderboard = async () => {
    try {
      const res = await fetch(`${API_BASE}/leaderboard`);
      if (res.ok) {
        const data = await res.json();
        setLeaderboard(data);
        setServerOnline(true);
      }
    } catch (e) {}
  };

  const fetchSubmissions = async () => {
    try {
      const res = await fetch(`${API_BASE}/submissions`);
      if (res.ok) {
        const data = await res.json();
        setSubmissions(data);
      }
    } catch (e) {}
  };

  const handleAuthSubmit = async (e) => {
    e.preventDefault();
    setAuthError('');
    if (!authUsername.trim()) {
      setAuthError('Username cannot be empty.');
      return;
    }

    try {
      const endpoint = `${API_BASE}/login`;
      const res = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username: authUsername.trim(), password: authPassword })
      });

      if (res.ok) {
        const data = await res.json();
        setCurrentUser(data.username);
        setCurrentUserRole(data.role || 'participant');
        setSessionToken(data.session_token || '');
        setAdminToken(data.admin_token || '');
        localStorage.setItem('contest_user', data.username);
        localStorage.setItem('contest_role', data.role || 'participant');
        if (data.session_token) localStorage.setItem('contest_session_token', data.session_token);
        if (data.admin_token) {
          localStorage.setItem('contest_admin_token', data.admin_token);
        } else {
          localStorage.removeItem('contest_admin_token');
        }
        setIsAuthModalOpen(false);
        setAuthPassword('');
        showToast(`Welcome, ${data.username}! (${data.role})`);
        if (data.role === 'admin') {
          setInAdminMode(true);
        }
      } else {
        const errData = await res.json().catch(() => ({}));
        setAuthError(errData.error || 'Invalid credentials. Please check your username and password.');
      }
    } catch (e) {
      setCurrentUser(authUsername.trim());
      setCurrentUserRole('participant');
      setAdminToken('');
      localStorage.setItem('contest_user', authUsername.trim());
      localStorage.setItem('contest_role', 'participant');
      setIsAuthModalOpen(false);
      showToast(`Logged in as ${authUsername.trim()}`);
    }
  };

  const handleLogout = () => {
    setCurrentUser('');
    setCurrentUserRole('participant');
    setSessionToken('');
    setAdminToken('');
    setInAdminMode(false);
    localStorage.removeItem('contest_user');
    localStorage.removeItem('contest_role');
    localStorage.removeItem('contest_session_token');
    localStorage.removeItem('contest_admin_token');
    showToast('You have been logged out.');
  };

  // Run Code against Sample or Custom Input (No score modification)
  const handleRunCode = async () => {
    if (!selectedProblem) return;
    setIsRunning(true);
    setRunResult(null);
    setSubmissionResult(null);

    try {
      const res = await fetch(`${API_BASE}/run`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          problem_id: selectedProblem.id,
          code: code,
          custom_input: useCustomInput ? customInput : ''
        })
      });

      if (res.ok) {
        const data = await res.json();
        setRunResult(data);
      } else {
        const err = await res.json().catch(() => ({ error: 'Run execution failed' }));
        showToast(err.error || 'Failed to run code', 'error');
      }
    } catch (e) {
      showToast('Network error while running code.', 'error');
    } finally {
      setIsRunning(false);
    }
  };

  // Submit Code for Official Hidden Test Case Judging
  const handleSubmitCode = async () => {
    if (!selectedProblem) return;
    if (!currentUser) {
      setIsAuthModalOpen(true);
      return;
    }

    setIsSubmitting(true);
    setSubmissionResult(null);
    setRunResult(null);

    try {
      const res = await fetch(`${API_BASE}/submit`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          username: currentUser,
          problem_id: selectedProblem.id,
          code: code,
          session_token: sessionToken
        })
      });

      if (res.ok) {
        const data = await res.json();
        setSubmissionResult(data);
        fetchLeaderboard();
        fetchSubmissions();
        if (data.verdict.includes('ACCEPTED')) {
          showToast(`Problem #${selectedProblem.id} Accepted! Score updated.`, 'success');
        } else {
          showToast(`Verdict: ${data.verdict}`, 'error');
        }
      } else {
        const err = await res.json().catch(() => ({ error: 'Submission rejected' }));
        showToast(err.error || 'Submission failed.', 'error');
      }
    } catch (e) {
      showToast('Network error while submitting code.', 'error');
    } finally {
      setIsSubmitting(false);
    }
  };



  const copyToClipboard = (text) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  const filteredProblems = problems.filter(p => 
    p.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
    p.description.toLowerCase().includes(searchQuery.toLowerCase()) ||
    p.id.toString() === searchQuery.trim()
  );

  const filteredLeaderboard = leaderboard.filter(entry => 
    entry.username.toLowerCase().includes(lbSearch.toLowerCase())
  );

  const mySubmissions = submissions.filter(s => s.username === currentUser || currentUserRole === 'admin');

  // If in Admin Mode, render the dedicated Admin Console
  if (inAdminMode) {
    return (
      <AdminPanel 
        adminToken={adminToken} 
        onExitAdmin={() => setInAdminMode(false)} 
        onLogout={handleLogout} 
      />
    );
  }

  return (
    <div style={{ minHeight: '100vh', display: 'flex', flexDirection: 'column', background: 'var(--bg-primary)' }}>
      
      {/* Toast Notification */}
      {notification && (
        <div style={{
          position: 'fixed',
          top: '20px',
          right: '24px',
          zIndex: 100,
          background: notification.type === 'success' ? '#15803D' : notification.type === 'error' ? '#B91C1C' : '#B45309',
          color: '#FFFFFF',
          padding: '10px 18px',
          borderRadius: '6px',
          boxShadow: '0 4px 12px rgba(0,0,0,0.15)',
          display: 'flex',
          alignItems: 'center',
          gap: '8px',
          fontSize: '0.88rem',
          fontWeight: 600
        }}>
          {notification.type === 'success' ? <CheckCircle2 size={16} /> : <AlertCircle size={16} />}
          {notification.message}
        </div>
      )}

      {/* Modern Professional Header */}
      <header style={{
        background: '#FFFFFF',
        borderBottom: '1px solid var(--border-color)',
        padding: '0 24px',
        height: '60px',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        position: 'sticky',
        top: 0,
        zIndex: 40
      }}>
        {/* Brand Logo */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px', cursor: 'pointer' }} onClick={() => setActiveTab('home')}>
          <div style={{
            background: 'var(--primary-blue)',
            width: '32px',
            height: '32px',
            borderRadius: '6px',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: '#FFFFFF'
          }}>
            <Code2 size={18} />
          </div>
          <div>
            <h1 style={{ fontSize: '1.05rem', fontWeight: 700, color: 'var(--text-main)', letterSpacing: '-0.01em' }}>
              CODE CONTEST SERVER
            </h1>
            <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)', display: 'flex', alignItems: 'center', gap: '5px' }}>
              <span style={{
                width: '6px',
                height: '6px',
                borderRadius: '50%',
                background: serverOnline ? '#10b981' : '#ef4444'
              }}></span>
              <span>{serverOnline ? 'Judge Online' : 'Connecting to Server...'}</span>
            </div>
          </div>
        </div>

        {/* Navigation Tabs */}
        <nav style={{ display: 'flex', gap: '4px' }}>
          {[
            { id: 'home', label: 'Problems', icon: Sparkles },
            { id: 'editor', label: 'Code Editor', icon: Code2 },
            { id: 'leaderboard', label: 'Leaderboard', icon: Trophy },
            { id: 'submissions', label: 'My Submissions', icon: FileCode }
          ].map(tab => {
            const Icon = tab.icon;
            const active = activeTab === tab.id;
            return (
              <button 
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  gap: '6px',
                  padding: '7px 14px',
                  borderRadius: '5px',
                  border: active ? '1px solid #DBEAFE' : '1px solid transparent',
                  background: active ? '#EFF6FF' : 'transparent',
                  color: active ? '#1D4ED8' : 'var(--text-muted)',
                  fontWeight: active ? 700 : 500,
                  fontSize: '0.85rem'
                }}>
                <Icon size={15} /> {tab.label}
              </button>
            );
          })}
        </nav>

        {/* User Account Widget */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          {currentUser ? (
            <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
              {currentUserRole === 'admin' && (
                <button
                  onClick={() => setInAdminMode(true)}
                  style={{
                    background: '#FEF3C7',
                    color: '#B45309',
                    border: '1px solid #FDE68A',
                    padding: '5px 12px',
                    borderRadius: '5px',
                    fontSize: '0.82rem',
                    fontWeight: 600,
                    display: 'flex',
                    alignItems: 'center',
                    gap: '5px'
                  }}>
                  <ShieldAlert size={14} /> Admin Console
                </button>
              )}

              <div style={{
                background: '#F1F5F9',
                color: 'var(--text-main)',
                padding: '5px 12px',
                borderRadius: '5px',
                fontSize: '0.82rem',
                fontWeight: 600,
                display: 'flex',
                alignItems: 'center',
                gap: '6px',
                border: '1px solid var(--border-color)'
              }}>
                <UserCheck size={14} color="var(--primary-blue)" /> {currentUser}
                <span style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>({currentUserRole})</span>
              </div>
              <button 
                onClick={handleLogout}
                title="Logout"
                style={{
                  background: '#FFFFFF',
                  color: 'var(--text-muted)',
                  border: '1px solid var(--border-color)',
                  padding: '5px 8px',
                  borderRadius: '5px',
                  display: 'flex',
                  alignItems: 'center'
                }}>
                <LogOut size={14} />
              </button>
            </div>
          ) : (
            <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
              <button 
                onClick={() => {
                  setAuthMode('login');
                  setAuthUsername('admin');
                  setAuthPassword('admin123');
                  setIsAuthModalOpen(true);
                }}
                style={{
                  background: '#FFFFFF',
                  color: 'var(--text-main)',
                  border: '1px solid var(--border-color)',
                  padding: '6px 12px',
                  borderRadius: '5px',
                  fontSize: '0.82rem',
                  fontWeight: 600,
                  display: 'flex',
                  alignItems: 'center',
                  gap: '5px'
                }}>
                <ShieldAlert size={14} /> Admin Login
              </button>

              <button 
                onClick={() => {
                  setAuthMode('login');
                  setIsAuthModalOpen(true);
                }}
                style={{
                  background: 'var(--primary-blue)',
                  color: '#FFFFFF',
                  border: 'none',
                  padding: '6px 14px',
                  borderRadius: '5px',
                  fontSize: '0.82rem',
                  fontWeight: 600,
                  display: 'flex',
                  alignItems: 'center',
                  gap: '5px'
                }}>
                <LogIn size={14} /> Sign In
              </button>
            </div>
          )}
        </div>
      </header>

      {/* Main Content Area */}
      <main style={{ flex: 1, padding: '24px', maxWidth: '1440px', width: '100%', margin: '0 auto' }}>
        
        {/* ================================================================= */}
        {/* TAB 1: CONTEST & PROBLEMS HOME                                    */}
        {/* ================================================================= */}
        {activeTab === 'home' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '20px' }}>
            
            {/* Contest Header Banner */}
            <div className="cp-card" style={{ padding: '24px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', flexWrap: 'wrap', gap: '16px' }}>
                <div>
                  <div style={{ display: 'flex', gap: '6px', marginBottom: '8px' }}>
                    <span className="badge badge-green">LIVE CONTEST</span>
                    <span className="badge badge-blue">180 MINUTES</span>
                    <span className="badge badge-purple">SANDBOXED C++ JUDGE</span>
                  </div>
                  <h2 style={{ fontSize: '1.6rem', fontWeight: 700, color: 'var(--text-main)', marginBottom: '8px' }}>
                    Operating Systems Coding Contest
                  </h2>
                  <p style={{ color: 'var(--text-muted)', fontSize: '0.9rem', maxWidth: '700px' }}>
                    Solve problems evaluated with CPU time and memory sandboxing. Rankings update dynamically on the leaderboard.
                  </p>
                </div>

                <div style={{ display: 'flex', gap: '10px' }}>
                  <button 
                    onClick={() => {
                      if (problems.length > 0) setSelectedProblem(problems[0]);
                      setActiveTab('editor');
                    }}
                    style={{
                      background: 'var(--primary-blue)',
                      color: '#FFFFFF',
                      padding: '9px 18px',
                      borderRadius: '5px',
                      border: 'none',
                      fontWeight: 600,
                      fontSize: '0.88rem',
                      display: 'flex',
                      alignItems: 'center',
                      gap: '6px'
                    }}>
                    <Play size={15} /> Start Solving
                  </button>
                  <button 
                    onClick={() => setActiveTab('leaderboard')}
                    style={{
                      background: '#FFFFFF',
                      color: 'var(--text-main)',
                      padding: '9px 16px',
                      borderRadius: '5px',
                      border: '1px solid var(--border-color)',
                      fontWeight: 600,
                      fontSize: '0.88rem',
                      display: 'flex',
                      alignItems: 'center',
                      gap: '6px'
                    }}>
                    <Trophy size={15} /> Standings
                  </button>
                </div>
              </div>
            </div>

            {/* Problem Search Bar */}
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <h3 style={{ fontSize: '1.2rem', fontWeight: 700, color: 'var(--text-main)', display: 'flex', alignItems: 'center', gap: '6px' }}>
                <Code2 size={18} color="var(--primary-blue)" /> Problems ({problems.length})
              </h3>

              <div style={{ position: 'relative', width: '260px' }}>
                <Search size={15} style={{ position: 'absolute', left: '10px', top: '10px', color: 'var(--text-dim)' }} />
                <input 
                  type="text" 
                  placeholder="Filter problems..."
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  style={{ width: '100%', paddingLeft: '32px', fontSize: '0.82rem' }}
                />
              </div>
            </div>

            {/* Problems Clean Table */}
            <div className="cp-card" style={{ overflow: 'hidden' }}>
              <table>
                <thead>
                  <tr>
                    <th style={{ padding: '12px 18px', width: '60px' }}>#</th>
                    <th style={{ padding: '12px 18px' }}>PROBLEM TITLE</th>
                    <th style={{ padding: '12px 18px' }}>DIFFICULTY</th>
                    <th style={{ padding: '12px 18px' }}>POINTS</th>
                    <th style={{ padding: '12px 18px' }}>RESOURCE LIMITS</th>
                    <th style={{ padding: '12px 18px', textAlign: 'right' }}>ACTION</th>
                  </tr>
                </thead>
                <tbody>
                  {filteredProblems.map((p) => {
                    const diffBadge = p.difficulty === 'Easy' ? 'badge-green' : p.difficulty === 'Hard' ? 'badge-red' : 'badge-yellow';
                    return (
                      <tr key={p.id}>
                        <td style={{ padding: '14px 18px', fontWeight: 600, color: 'var(--text-muted)' }}>
                          P{p.id}
                        </td>
                        <td style={{ padding: '14px 18px' }}>
                          <div style={{ fontWeight: 600, color: 'var(--text-main)', fontSize: '0.92rem' }}>{p.title}</div>
                          <div style={{ fontSize: '0.78rem', color: 'var(--text-muted)', maxWidth: '420px', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                            {p.description}
                          </div>
                        </td>
                        <td style={{ padding: '14px 18px' }}>
                          <span className={`badge ${diffBadge}`}>{p.difficulty || 'Medium'}</span>
                        </td>
                        <td style={{ padding: '14px 18px', fontWeight: 700, color: 'var(--primary-blue)' }}>
                          {p.points} PTS
                        </td>
                        <td style={{ padding: '14px 18px', fontSize: '0.8rem', color: 'var(--text-muted)' }}>
                          {p.time_limit_sec}s CPU • {p.memory_limit_mb}MB RAM
                        </td>
                        <td style={{ padding: '14px 18px', textAlign: 'right' }}>
                          <button 
                            onClick={() => {
                              setSelectedProblem(p);
                              setCode(PROBLEM_TEMPLATES[p.id] || PROBLEM_TEMPLATES[1]);
                              setActiveTab('editor');
                            }}
                            style={{
                              background: '#EFF6FF',
                              color: 'var(--primary-blue)',
                              border: '1px solid #DBEAFE',
                              padding: '5px 12px',
                              borderRadius: '4px',
                              fontSize: '0.82rem',
                              fontWeight: 600,
                              display: 'inline-flex',
                              alignItems: 'center',
                              gap: '4px'
                            }}>
                            Solve <Play size={12} />
                          </button>
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          </div>
        )}

        {/* ================================================================= */}
        {/* TAB 2: CODE EDITOR & LIVE EVALUATION                              */}
        {/* ================================================================= */}
        {activeTab === 'editor' && (
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1.35fr', gap: '16px', height: 'calc(100vh - 110px)' }}>
            
            {/* Left: Problem Details & Constraints */}
            <div className="cp-card" style={{ padding: '20px', display: 'flex', flexDirection: 'column', overflowY: 'auto' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '14px', flexWrap: 'wrap', gap: '8px' }}>
                <select 
                  value={selectedProblem?.id || 1}
                  onChange={(e) => {
                    const prob = problems.find(p => p.id === parseInt(e.target.value));
                    if (prob) {
                      setSelectedProblem(prob);
                      setCode(PROBLEM_TEMPLATES[prob.id] || PROBLEM_TEMPLATES[1]);
                    }
                  }}
                  style={{ fontWeight: 600, fontSize: '0.88rem' }}>
                  {problems.map(p => (
                    <option key={p.id} value={p.id}>P{p.id}: {p.title} ({p.points} pts)</option>
                  ))}
                </select>

                <div style={{ display: 'flex', gap: '6px' }}>
                  <span className="badge badge-purple">{selectedProblem?.points || 100} PTS</span>
                  <span className="badge badge-blue">{selectedProblem?.time_limit_sec || 3.0}s CPU</span>
                  <span className="badge badge-yellow">{selectedProblem?.memory_limit_mb || 64}MB RAM</span>
                </div>
              </div>

              <div style={{ flex: 1 }}>
                <h3 style={{ fontSize: '1.25rem', fontWeight: 700, marginBottom: '12px', color: 'var(--text-main)' }}>
                  {selectedProblem?.title}
                </h3>
                
                <div style={{
                  background: '#F8FAFC',
                  padding: '14px',
                  borderRadius: '5px',
                  border: '1px solid var(--border-color)',
                  whiteSpace: 'pre-wrap',
                  fontSize: '0.88rem',
                  color: 'var(--text-main)',
                  lineHeight: 1.6,
                  marginBottom: '16px'
                }}>
                  {selectedProblem?.description}
                </div>

                {selectedProblem?.input_format && (
                  <div style={{ marginBottom: '14px' }}>
                    <div style={{ fontSize: '0.8rem', fontWeight: 700, color: 'var(--text-muted)', textTransform: 'uppercase', marginBottom: '4px' }}>Input Format</div>
                    <div style={{ fontSize: '0.85rem', color: 'var(--text-main)' }}>{selectedProblem.input_format}</div>
                  </div>
                )}

                {selectedProblem?.output_format && (
                  <div style={{ marginBottom: '14px' }}>
                    <div style={{ fontSize: '0.8rem', fontWeight: 700, color: 'var(--text-muted)', textTransform: 'uppercase', marginBottom: '4px' }}>Output Format</div>
                    <div style={{ fontSize: '0.85rem', color: 'var(--text-main)' }}>{selectedProblem.output_format}</div>
                  </div>
                )}

                {selectedProblem?.constraints && (
                  <div style={{ marginBottom: '14px' }}>
                    <div style={{ fontSize: '0.8rem', fontWeight: 700, color: 'var(--text-muted)', textTransform: 'uppercase', marginBottom: '4px' }}>Constraints</div>
                    <div className="mono-font" style={{ fontSize: '0.82rem', background: '#F1F5F9', padding: '6px 10px', borderRadius: '4px', color: 'var(--text-main)' }}>
                      {selectedProblem.constraints}
                    </div>
                  </div>
                )}

                {/* Sample Test Case */}
                {selectedProblem?.sample_input && (
                  <div style={{ marginBottom: '16px' }}>
                    <div style={{ fontSize: '0.8rem', fontWeight: 700, color: 'var(--text-muted)', textTransform: 'uppercase', marginBottom: '6px' }}>Sample Test Case</div>
                    <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '8px' }}>
                      <div>
                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)', marginBottom: '2px' }}>Sample Input</div>
                        <pre className="mono-font" style={{ background: '#F1F5F9', padding: '8px 10px', borderRadius: '4px', fontSize: '0.82rem', border: '1px solid var(--border-color)' }}>
                          {selectedProblem.sample_input}
                        </pre>
                      </div>
                      <div>
                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)', marginBottom: '2px' }}>Sample Output</div>
                        <pre className="mono-font" style={{ background: '#F1F5F9', padding: '8px 10px', borderRadius: '4px', fontSize: '0.82rem', border: '1px solid var(--border-color)' }}>
                          {selectedProblem.sample_output}
                        </pre>
                      </div>
                    </div>
                  </div>
                )}

                {/* Custom Input Toggle */}
                <div style={{ marginTop: '12px', borderTop: '1px solid var(--border-color)', paddingTop: '12px' }}>
                  <label style={{ display: 'flex', alignItems: 'center', gap: '6px', cursor: 'pointer', fontSize: '0.82rem', fontWeight: 600, color: 'var(--primary-blue)' }}>
                    <input 
                      type="checkbox" 
                      checked={useCustomInput} 
                      onChange={(e) => setUseCustomInput(e.target.checked)} 
                    />
                    Test with Custom Input
                  </label>

                  {useCustomInput && (
                    <textarea 
                      value={customInput}
                      onChange={(e) => setCustomInput(e.target.value)}
                      placeholder="Enter custom stdin test input..."
                      className="mono-font"
                      style={{ width: '100%', height: '70px', marginTop: '6px', fontSize: '0.82rem' }}
                    />
                  )}
                </div>
              </div>
            </div>

            {/* Right: Code Editor & Execution Results */}
            <div style={{ display: 'flex', flexDirection: 'column', gap: '12px', height: '100%' }}>
              
              {/* Editor Bar */}
              <div className="cp-card" style={{ padding: '8px 14px', display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: '8px' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
                  <span style={{ fontSize: '0.78rem', fontWeight: 600, color: 'var(--text-muted)' }}>C++14 (GCC -O2)</span>
                  <div style={{ height: '14px', width: '1px', background: 'var(--border-color)' }}></div>
                  <button 
                    onClick={() => setCode(PROBLEM_TEMPLATES[selectedProblem?.id] || PROBLEM_TEMPLATES[1])}
                    style={{ background: '#DCFCE7', color: '#15803D', border: '1px solid #BBF7D0', padding: '3px 8px', borderRadius: '3px', fontSize: '0.72rem', fontWeight: 600 }}>
                    AC Solution
                  </button>
                  <button 
                    onClick={() => setCode(COMMON_ERROR_TEMPLATES.wa)}
                    style={{ background: '#FEE2E2', color: '#B91C1C', border: '1px solid #FECACA', padding: '3px 8px', borderRadius: '3px', fontSize: '0.72rem', fontWeight: 600 }}>
                    WA Test
                  </button>
                  <button 
                    onClick={() => setCode(COMMON_ERROR_TEMPLATES.tle)}
                    style={{ background: '#FEF3C7', color: '#B45309', border: '1px solid #FDE68A', padding: '3px 8px', borderRadius: '3px', fontSize: '0.72rem', fontWeight: 600 }}>
                    TLE Test
                  </button>
                </div>

                <div style={{ display: 'flex', gap: '8px' }}>
                  {/* Run Code Button */}
                  <button 
                    onClick={handleRunCode}
                    disabled={isRunning || isSubmitting}
                    style={{
                      background: '#FFFFFF',
                      color: 'var(--text-main)',
                      border: '1px solid var(--border-color)',
                      padding: '6px 12px',
                      borderRadius: '4px',
                      fontWeight: 600,
                      fontSize: '0.82rem',
                      display: 'flex',
                      alignItems: 'center',
                      gap: '5px'
                    }}>
                    <Play size={13} color="var(--primary-blue)" />
                    {isRunning ? 'Running...' : 'Run Code'}
                  </button>

                  {/* Submit Button */}
                  <button 
                    onClick={handleSubmitCode}
                    disabled={isSubmitting || isRunning}
                    style={{
                      background: '#15803D',
                      color: '#FFFFFF',
                      border: 'none',
                      padding: '6px 16px',
                      borderRadius: '4px',
                      fontWeight: 600,
                      fontSize: '0.82rem',
                      display: 'flex',
                      alignItems: 'center',
                      gap: '5px'
                    }}>
                    <CheckCircle2 size={14} />
                    {isSubmitting ? 'Evaluating...' : 'Submit'}
                  </button>
                </div>
              </div>

              {/* Professional Code Editor Area */}
              <div className="cp-card" style={{ flex: 1, display: 'flex', flexDirection: 'column', overflow: 'hidden' }}>
                <textarea 
                  value={code}
                  onChange={(e) => setCode(e.target.value)}
                  className="mono-font"
                  style={{
                    flex: 1,
                    width: '100%',
                    background: '#1E293B',
                    color: '#F8FAFC',
                    border: 'none',
                    padding: '14px',
                    fontSize: '0.88rem',
                    lineHeight: '1.5',
                    resize: 'none',
                    outline: 'none',
                    borderRadius: '6px'
                  }}
                  spellCheck="false"
                />
              </div>

              {/* Execution Feedback / Results */}
              {(runResult || submissionResult) && (
                <div className="cp-card" style={{ padding: '14px', maxHeight: '200px', overflowY: 'auto' }}>
                  {runResult && (
                    <div>
                      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '8px' }}>
                        <span style={{ fontSize: '0.82rem', fontWeight: 700, color: 'var(--text-muted)' }}>RUN RESULT</span>
                        <div style={{ display: 'flex', gap: '8px' }}>
                          <span className={`badge ${runResult.verdict.includes('ACCEPTED') ? 'badge-green' : 'badge-red'}`}>
                            {runResult.verdict}
                          </span>
                          <span style={{ fontSize: '0.78rem', color: 'var(--text-muted)' }}>
                            {runResult.execution_time_ms} ms • {runResult.memory_used_kb} KB
                          </span>
                        </div>
                      </div>
                      {runResult.runtime_output && (
                        <div style={{ marginTop: '6px' }}>
                          <div style={{ fontSize: '0.72rem', color: 'var(--text-muted)' }}>Program Output:</div>
                          <pre className="mono-font" style={{ background: '#F1F5F9', padding: '6px 10px', borderRadius: '4px', fontSize: '0.8rem', marginTop: '2px' }}>
                            {runResult.runtime_output}
                          </pre>
                        </div>
                      )}
                    </div>
                  )}

                  {submissionResult && (
                    <div>
                      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '8px' }}>
                        <span style={{ fontSize: '0.82rem', fontWeight: 700, color: 'var(--text-muted)' }}>
                          OFFICIAL SUBMISSION #{submissionResult.submission_id}
                        </span>
                        <div style={{ display: 'flex', gap: '8px' }}>
                          <span className={`badge ${submissionResult.verdict.includes('ACCEPTED') ? 'badge-green' : 'badge-red'}`}>
                            {submissionResult.verdict}
                          </span>
                          <span style={{ fontSize: '0.78rem', color: 'var(--text-muted)' }}>
                            {submissionResult.execution_time_ms} ms • {submissionResult.memory_used_kb} KB
                          </span>
                        </div>
                      </div>
                      {submissionResult.compiler_output && (
                        <div style={{ marginTop: '6px' }}>
                          <div style={{ fontSize: '0.72rem', color: '#B91C1C' }}>Compiler Messages:</div>
                          <pre className="mono-font" style={{ background: '#FEE2E2', padding: '6px 10px', borderRadius: '4px', fontSize: '0.8rem', color: '#B91C1C', marginTop: '2px' }}>
                            {submissionResult.compiler_output}
                          </pre>
                        </div>
                      )}
                    </div>
                  )}
                </div>
              )}

            </div>
          </div>
        )}

        {/* ================================================================= */}
        {/* TAB 3: LEADERBOARD                                                */}
        {/* ================================================================= */}
        {activeTab === 'leaderboard' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
            <div className="cp-card" style={{ padding: '18px 24px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <div>
                <h3 style={{ fontSize: '1.25rem', fontWeight: 700, color: 'var(--text-main)', display: 'flex', alignItems: 'center', gap: '6px' }}>
                  <Trophy size={20} color="#D97706" /> Live Contest Leaderboard
                </h3>
                <p style={{ fontSize: '0.82rem', color: 'var(--text-muted)' }}>
                  Rankings update dynamically via synchronized reader-writer locks.
                </p>
              </div>

              <div style={{ display: 'flex', gap: '10px' }}>
                <input 
                  type="text" 
                  placeholder="Filter participants..."
                  value={lbSearch}
                  onChange={(e) => setLbSearch(e.target.value)}
                  style={{ width: '220px', fontSize: '0.82rem' }}
                />
                <button 
                  onClick={fetchLeaderboard}
                  style={{
                    background: '#FFFFFF',
                    color: 'var(--text-main)',
                    border: '1px solid var(--border-color)',
                    padding: '6px 12px',
                    borderRadius: '4px',
                    fontSize: '0.82rem',
                    fontWeight: 600,
                    display: 'flex',
                    alignItems: 'center',
                    gap: '4px'
                  }}>
                  <RefreshCw size={12} /> Refresh
                </button>
              </div>
            </div>

            {/* Leaderboard Table */}
            <div className="cp-card" style={{ overflow: 'hidden' }}>
              <table>
                <thead>
                  <tr>
                    <th style={{ padding: '12px 18px', width: '80px' }}>RANK</th>
                    <th style={{ padding: '12px 18px' }}>PARTICIPANT</th>
                    <th style={{ padding: '12px 18px' }}>SOLVED</th>
                    <th style={{ padding: '12px 18px' }}>SCORE</th>
                    <th style={{ padding: '12px 18px' }}>PENALTY TIME</th>
                  </tr>
                </thead>
                <tbody>
                  {filteredLeaderboard.length === 0 ? (
                    <tr>
                      <td colSpan="5" style={{ padding: '32px', textAlign: 'center', color: 'var(--text-muted)' }}>
                        No submissions recorded on the leaderboard yet.
                      </td>
                    </tr>
                  ) : (
                    filteredLeaderboard.map((row) => (
                      <tr key={row.username} style={{ 
                        background: row.username === currentUser ? '#EFF6FF' : 'transparent'
                      }}>
                        <td style={{ padding: '12px 18px', fontWeight: 700 }}>
                          {row.rank === 1 && <span style={{ color: '#D97706' }}>🥇 1</span>}
                          {row.rank === 2 && <span style={{ color: '#64748B' }}>🥈 2</span>}
                          {row.rank === 3 && <span style={{ color: '#B45309' }}>🥉 3</span>}
                          {row.rank > 3 && <span>#{row.rank}</span>}
                        </td>
                        <td style={{ padding: '12px 18px', fontWeight: 600, color: row.username === currentUser ? 'var(--primary-blue)' : 'var(--text-main)' }}>
                          {row.username} {row.username === currentUser && '(You)'}
                        </td>
                        <td style={{ padding: '12px 18px' }}>
                          <span className="badge badge-green">{row.solved_count} Solved</span>
                        </td>
                        <td style={{ padding: '12px 18px', fontWeight: 700, color: 'var(--primary-blue)' }}>
                          {row.score} PTS
                        </td>
                        <td style={{ padding: '12px 18px', color: 'var(--text-muted)', fontFamily: 'var(--font-mono)', fontSize: '0.85rem' }}>
                          {row.penalty_sec}s
                        </td>
                      </tr>
                    ))
                  )}
                </tbody>
              </table>
            </div>
          </div>
        )}

        {/* ================================================================= */}
        {/* TAB 4: MY SUBMISSIONS HISTORY                                     */}
        {/* ================================================================= */}
        {activeTab === 'submissions' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
            <div className="cp-card" style={{ padding: '18px 24px', display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
              <div>
                <h3 style={{ fontSize: '1.25rem', fontWeight: 700, color: 'var(--text-main)', display: 'flex', alignItems: 'center', gap: '6px' }}>
                  <FileCode size={20} color="var(--primary-blue)" /> Submissions Audit Log
                </h3>
                <p style={{ fontSize: '0.82rem', color: 'var(--text-muted)' }}>
                  Detailed history of code evaluated in isolated processes.
                </p>
              </div>

              <button 
                onClick={fetchSubmissions}
                style={{
                  background: '#FFFFFF',
                  color: 'var(--text-main)',
                  border: '1px solid var(--border-color)',
                  padding: '6px 12px',
                  borderRadius: '4px',
                  fontSize: '0.82rem',
                  fontWeight: 600,
                  display: 'flex',
                  alignItems: 'center',
                  gap: '4px'
                }}>
                <RefreshCw size={12} /> Refresh
              </button>
            </div>

            {/* Submissions Table */}
            <div className="cp-card" style={{ overflow: 'hidden' }}>
              <table>
                <thead>
                  <tr>
                    <th style={{ padding: '12px 18px' }}>ID</th>
                    <th style={{ padding: '12px 18px' }}>USER</th>
                    <th style={{ padding: '12px 18px' }}>PROBLEM</th>
                    <th style={{ padding: '12px 18px' }}>VERDICT</th>
                    <th style={{ padding: '12px 18px' }}>RUNTIME</th>
                    <th style={{ padding: '12px 18px' }}>MEMORY</th>
                    <th style={{ padding: '12px 18px', textAlign: 'right' }}>ACTION</th>
                  </tr>
                </thead>
                <tbody>
                  {mySubmissions.length === 0 ? (
                    <tr>
                      <td colSpan="7" style={{ padding: '32px', textAlign: 'center', color: 'var(--text-muted)' }}>
                        No submissions recorded yet.
                      </td>
                    </tr>
                  ) : (
                    mySubmissions.map((s) => (
                      <tr key={s.id}>
                        <td style={{ padding: '12px 18px', fontFamily: 'var(--font-mono)', fontWeight: 600, fontSize: '0.85rem' }}>
                          #{s.id}
                        </td>
                        <td style={{ padding: '12px 18px', fontWeight: 600, color: 'var(--primary-blue)' }}>
                          {s.username}
                        </td>
                        <td style={{ padding: '12px 18px', fontSize: '0.85rem' }}>
                          #{s.problem_id}: {s.problem_title}
                        </td>
                        <td style={{ padding: '12px 18px' }}>
                          <span className={`badge ${s.verdict.includes('ACCEPTED') ? 'badge-green' : s.verdict.includes('TIME') ? 'badge-yellow' : 'badge-red'}`}>
                            {s.verdict}
                          </span>
                        </td>
                        <td style={{ padding: '12px 18px', fontFamily: 'var(--font-mono)', fontSize: '0.82rem' }}>
                          {s.execution_time_ms} ms
                        </td>
                        <td style={{ padding: '12px 18px', fontFamily: 'var(--font-mono)', fontSize: '0.82rem' }}>
                          {s.memory_used_kb} KB
                        </td>
                        <td style={{ padding: '12px 18px', textAlign: 'right' }}>
                          <button 
                            onClick={() => setViewingSubmission(s)}
                            style={{
                              background: '#EFF6FF',
                              color: 'var(--primary-blue)',
                              border: '1px solid #DBEAFE',
                              padding: '3px 8px',
                              borderRadius: '3px',
                              fontSize: '0.78rem',
                              fontWeight: 600,
                              display: 'inline-flex',
                              alignItems: 'center',
                              gap: '3px'
                            }}>
                            <Eye size={12} /> Inspect
                          </button>
                        </td>
                      </tr>
                    ))
                  )}
                </tbody>
              </table>
            </div>
          </div>
        )}

      </main>

      {/* =================================================================== */}
      {/* AUTHENTICATION MODAL (LOGIN / REGISTER)                             */}
      {/* =================================================================== */}
      {isAuthModalOpen && (
        <div style={{
          position: 'fixed',
          inset: 0,
          background: 'rgba(0,0,0,0.4)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 90
        }}>
          <div className="cp-card" style={{ width: '420px', padding: '28px', position: 'relative' }}>
            <h3 style={{ fontSize: '1.3rem', fontWeight: 700, color: 'var(--text-main)', marginBottom: '8px' }}>
              Participant Sign In
            </h3>
            <p style={{ fontSize: '0.82rem', color: 'var(--text-muted)', marginBottom: '18px' }}>
              Log in to submit code and record scores on the leaderboard.
            </p>

            {authError && (
              <div style={{
                background: '#FEE2E2',
                color: '#B91C1C',
                border: '1px solid #FECACA',
                padding: '8px 12px',
                borderRadius: '4px',
                fontSize: '0.82rem',
                marginBottom: '14px'
              }}>
                {authError}
              </div>
            )}

            <form onSubmit={handleAuthSubmit} style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
              <div>
                <label style={{ fontSize: '0.8rem', fontWeight: 600, color: 'var(--text-main)', display: 'block', marginBottom: '4px' }}>Username</label>
                <input 
                  type="text" 
                  value={authUsername}
                  onChange={(e) => setAuthUsername(e.target.value)}
                  placeholder="e.g. alice_coder"
                  style={{ width: '100%' }}
                  required
                />
              </div>

              <div>
                <label style={{ fontSize: '0.8rem', fontWeight: 600, color: 'var(--text-main)', display: 'block', marginBottom: '4px' }}>Password</label>
                <input 
                  type="password" 
                  value={authPassword}
                  onChange={(e) => setAuthPassword(e.target.value)}
                  placeholder="Password"
                  style={{ width: '100%' }}
                  required
                />
              </div>

              <div style={{ display: 'flex', gap: '6px', margin: '6px 0' }}>
                <button
                  type="button"
                  onClick={() => { setAuthUsername('admin'); setAuthPassword('admin123'); }}
                  style={{ background: '#F1F5F9', border: '1px solid var(--border-color)', color: 'var(--text-main)', padding: '4px 8px', borderRadius: '4px', fontSize: '0.72rem' }}>
                  Admin (admin)
                </button>
                <button
                  type="button"
                  onClick={() => { setAuthUsername('alice_coder'); setAuthPassword('pass123'); }}
                  style={{ background: '#F1F5F9', border: '1px solid var(--border-color)', color: 'var(--text-main)', padding: '4px 8px', borderRadius: '4px', fontSize: '0.72rem' }}>
                  Student (alice_coder)
                </button>
              </div>

              <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '10px', marginTop: '10px' }}>
                <button
                  type="button"
                  onClick={() => setIsAuthModalOpen(false)}
                  style={{
                    background: '#FFFFFF',
                    border: '1px solid var(--border-color)',
                    color: 'var(--text-main)',
                    padding: '8px 16px',
                    borderRadius: '4px',
                    fontSize: '0.85rem'
                  }}>
                  Cancel
                </button>
                <button
                  type="submit"
                  style={{
                    background: 'var(--primary-blue)',
                    color: '#FFFFFF',
                    border: 'none',
                    padding: '8px 18px',
                    borderRadius: '4px',
                    fontWeight: 600,
                    fontSize: '0.85rem'
                  }}>
                  Sign In
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =================================================================== */}
      {/* VIEW SUBMISSION CODE MODAL                                          */}
      {/* =================================================================== */}
      {viewingSubmission && (
        <div style={{
          position: 'fixed',
          inset: 0,
          background: 'rgba(0,0,0,0.5)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 90
        }}>
          <div className="cp-card" style={{ width: '680px', maxHeight: '85vh', padding: '24px', display: 'flex', flexDirection: 'column' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '14px' }}>
              <div>
                <h3 style={{ fontSize: '1.15rem', fontWeight: 700, color: 'var(--text-main)' }}>
                  Submission #{viewingSubmission.id} ({viewingSubmission.username})
                </h3>
                <div style={{ fontSize: '0.78rem', color: 'var(--text-muted)' }}>
                  {viewingSubmission.problem_title} • Verdict: {viewingSubmission.verdict}
                </div>
              </div>
              <button
                onClick={() => setViewingSubmission(null)}
                style={{ background: '#F1F5F9', border: '1px solid var(--border-color)', padding: '4px 10px', borderRadius: '4px', fontSize: '0.8rem' }}>
                Close
              </button>
            </div>

            <pre className="mono-font" style={{
              flex: 1,
              background: '#1E293B',
              color: '#F8FAFC',
              padding: '14px',
              borderRadius: '5px',
              overflowY: 'auto',
              fontSize: '0.82rem',
              lineHeight: 1.5
            }}>
              {viewingSubmission.code}
            </pre>
          </div>
        </div>
      )}

    </div>
  );
}
