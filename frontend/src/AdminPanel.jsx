import React, { useState, useEffect } from 'react';
import { 
  LayoutDashboard, Trophy, Code2, Users, FileCode, LogOut,
  Plus, Edit2, Trash2, CheckCircle2, XCircle, AlertCircle, Search, 
  RefreshCw, ShieldAlert, Clock, Database, Ban, Check, Eye, X
} from 'lucide-react';

const API_BASE = 'http://localhost:8080/api';

export default function AdminPanel({ adminToken, currentUser, onExitAdmin, onLogout }) {
  const [activeTab, setActiveTab] = useState('dashboard'); // dashboard | contests | problems | participants | submissions
  const [loading, setLoading] = useState(false);
  const [notification, setNotification] = useState(null);

  // Data States
  const [stats, setStats] = useState(null);
  const [contests, setContests] = useState([]);
  const [problems, setProblems] = useState([]);
  const [participants, setParticipants] = useState([]);
  const [submissions, setSubmissions] = useState([]);

  // Search & Filter States
  const [searchQuery, setSearchQuery] = useState('');
  const [submissionFilter, setSubmissionFilter] = useState('ALL');

  // Modals & Active Edit States
  const [isContestModalOpen, setIsContestModalOpen] = useState(false);
  const [editingContest, setEditingContest] = useState(null);
  const [contestForm, setContestForm] = useState({ title: '', description: '', duration_minutes: 120, status: 'DRAFT' });

  const [isProblemModalOpen, setIsProblemModalOpen] = useState(false);
  const [editingProblem, setEditingProblem] = useState(null);
  const [problemForm, setProblemForm] = useState({
    title: '', description: '', difficulty: 'Medium', points: 100,
    time_limit_sec: 3.0, memory_limit_mb: 64,
    input_format: '', output_format: '', constraints: '',
    sample_input: '', sample_output: '',
    hidden_input: '', hidden_output: ''
  });

  const [deleteConfirm, setDeleteConfirm] = useState(null); // { type: 'contest'|'problem', id: number, name: string }
  const [viewingCodeModal, setViewingCodeModal] = useState(null);

  const showToast = (message, type = 'success') => {
    setNotification({ message, type });
    setTimeout(() => setNotification(null), 3500);
  };

  const getAuthHeaders = () => ({
    'Content-Type': 'application/json',
    'Authorization': `Bearer ${adminToken}`,
    'X-Admin-Token': adminToken
  });

  // Initial Load & Tab Switching
  useEffect(() => {
    if (activeTab === 'dashboard') fetchDashboardStats();
    else if (activeTab === 'contests') fetchContests();
    else if (activeTab === 'problems') fetchProblems();
    else if (activeTab === 'participants') fetchParticipants();
    else if (activeTab === 'submissions') fetchSubmissions();
  }, [activeTab]);

  // -------------------------------------------------------------------------
  // API Fetch Functions
  // -------------------------------------------------------------------------
  const fetchDashboardStats = async () => {
    setLoading(true);
    try {
      const res = await fetch(`${API_BASE}/admin/dashboard`, { headers: getAuthHeaders() });
      if (res.ok) {
        const data = await res.json();
        setStats(data);
      } else {
        showToast('Failed to load dashboard metrics.', 'error');
      }
    } catch (e) {
      showToast('Error connecting to backend.', 'error');
    } finally {
      setLoading(false);
    }
  };

  const fetchContests = async () => {
    setLoading(true);
    try {
      const res = await fetch(`${API_BASE}/admin/contests`, { headers: getAuthHeaders() });
      if (res.ok) setContests(await res.json());
    } catch (e) {
      showToast('Error loading contests.', 'error');
    } finally {
      setLoading(false);
    }
  };

  const fetchProblems = async () => {
    setLoading(true);
    try {
      const res = await fetch(`${API_BASE}/admin/problems`, { headers: getAuthHeaders() });
      if (res.ok) setProblems(await res.json());
    } catch (e) {
      showToast('Error loading problems.', 'error');
    } finally {
      setLoading(false);
    }
  };

  const fetchParticipants = async () => {
    setLoading(true);
    try {
      const res = await fetch(`${API_BASE}/admin/participants`, { headers: getAuthHeaders() });
      if (res.ok) setParticipants(await res.json());
    } catch (e) {
      showToast('Error loading participants.', 'error');
    } finally {
      setLoading(false);
    }
  };

  const fetchSubmissions = async () => {
    setLoading(true);
    try {
      const res = await fetch(`${API_BASE}/admin/submissions`, { headers: getAuthHeaders() });
      if (res.ok) setSubmissions(await res.json());
    } catch (e) {
      showToast('Error loading submissions.', 'error');
    } finally {
      setLoading(false);
    }
  };


  // -------------------------------------------------------------------------
  // Contest Operations
  // -------------------------------------------------------------------------
  const handleOpenContestModal = (contest = null) => {
    if (contest) {
      setEditingContest(contest);
      setContestForm({
        title: contest.title,
        description: contest.description,
        duration_minutes: contest.duration_minutes,
        status: contest.status
      });
    } else {
      setEditingContest(null);
      setContestForm({ title: '', description: '', duration_minutes: 120, status: 'DRAFT' });
    }
    setIsContestModalOpen(true);
  };

  const handleSaveContest = async (e) => {
    e.preventDefault();
    if (!contestForm.title.trim()) {
      showToast('Contest title is required.', 'error');
      return;
    }

    try {
      const method = editingContest ? 'PUT' : 'POST';
      const endpoint = editingContest ? `${API_BASE}/admin/contests/${editingContest.id}` : `${API_BASE}/admin/contests`;
      const res = await fetch(endpoint, {
        method,
        headers: getAuthHeaders(),
        body: JSON.stringify(contestForm)
      });

      if (res.ok) {
        showToast(editingContest ? 'Contest updated successfully!' : 'Contest created successfully!');
        setIsContestModalOpen(false);
        fetchContests();
      } else {
        showToast('Operation failed on server.', 'error');
      }
    } catch (e) {
      showToast('Communication error with server.', 'error');
    }
  };

  const handleUpdateContestStatus = async (id, status) => {
    try {
      const res = await fetch(`${API_BASE}/admin/contests/${id}/status`, {
        method: 'POST',
        headers: getAuthHeaders(),
        body: JSON.stringify({ status })
      });
      if (res.ok) {
        showToast(`Contest status changed to ${status}.`);
        fetchContests();
      }
    } catch (e) {
      showToast('Failed to update contest status.', 'error');
    }
  };

  const handleDeleteContest = async (id) => {
    try {
      const res = await fetch(`${API_BASE}/admin/contests/${id}`, {
        method: 'DELETE',
        headers: getAuthHeaders()
      });
      if (res.ok) {
        showToast('Contest deleted successfully.');
        setDeleteConfirm(null);
        fetchContests();
      }
    } catch (e) {
      showToast('Failed to delete contest.', 'error');
    }
  };

  // -------------------------------------------------------------------------
  // Problem Operations
  // -------------------------------------------------------------------------
  const handleOpenProblemModal = (problem = null) => {
    if (problem) {
      setEditingProblem(problem);
      setProblemForm({
        title: problem.title || '',
        description: problem.description || '',
        difficulty: problem.difficulty || 'Medium',
        points: problem.points || 100,
        time_limit_sec: problem.time_limit_sec || 3.0,
        memory_limit_mb: problem.memory_limit_mb || 64,
        input_format: problem.input_format || '',
        output_format: problem.output_format || '',
        constraints: problem.constraints || '',
        sample_input: problem.sample_input || '',
        sample_output: problem.sample_output || '',
        hidden_input: problem.hidden_input || problem.sample_input || '',
        hidden_output: problem.hidden_output || problem.sample_output || ''
      });
    } else {
      setEditingProblem(null);
      setProblemForm({
        title: '', description: '', difficulty: 'Medium', points: 100,
        time_limit_sec: 3.0, memory_limit_mb: 64,
        input_format: '', output_format: '', constraints: '',
        sample_input: '', sample_output: '',
        hidden_input: '', hidden_output: ''
      });
    }
    setIsProblemModalOpen(true);
  };

  const handleSaveProblem = async (e) => {
    e.preventDefault();
    if (!problemForm.title.trim()) {
      showToast('Problem title is required.', 'error');
      return;
    }

    try {
      const method = editingProblem ? 'PUT' : 'POST';
      const endpoint = editingProblem ? `${API_BASE}/admin/problems/${editingProblem.id}` : `${API_BASE}/admin/problems`;
      const res = await fetch(endpoint, {
        method,
        headers: getAuthHeaders(),
        body: JSON.stringify(problemForm)
      });

      if (res.ok) {
        showToast(editingProblem ? 'Problem updated successfully!' : 'Problem added successfully!');
        setIsProblemModalOpen(false);
        fetchProblems();
      } else {
        showToast('Problem save failed on server.', 'error');
      }
    } catch (e) {
      showToast('Communication error with server.', 'error');
    }
  };

  const handleDeleteProblem = async (id) => {
    try {
      const res = await fetch(`${API_BASE}/admin/problems/${id}`, {
        method: 'DELETE',
        headers: getAuthHeaders()
      });
      if (res.ok) {
        showToast('Problem deleted successfully.');
        setDeleteConfirm(null);
        fetchProblems();
      }
    } catch (e) {
      showToast('Failed to delete problem.', 'error');
    }
  };

  // -------------------------------------------------------------------------
  // Participant Operations (Suspend / Reactivate)
  // -------------------------------------------------------------------------
  const handleToggleSuspendUser = async (username, currentSuspended) => {
    try {
      const res = await fetch(`${API_BASE}/admin/participants/${username}/suspend`, {
        method: 'POST',
        headers: getAuthHeaders(),
        body: JSON.stringify({ suspended: !currentSuspended })
      });
      if (res.ok) {
        showToast(`User ${username} ${!currentSuspended ? 'suspended' : 'reactivated'}.`);
        fetchParticipants();
      } else {
        showToast('Cannot suspend master administrator.', 'error');
      }
    } catch (e) {
      showToast('Failed to update user status.', 'error');
    }
  };

  // -------------------------------------------------------------------------
  // Render Filters
  // -------------------------------------------------------------------------
  const filteredContests = contests.filter(c => 
    c.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
    c.status.toLowerCase().includes(searchQuery.toLowerCase())
  );

  const filteredProblems = problems.filter(p => 
    p.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
    p.difficulty.toLowerCase().includes(searchQuery.toLowerCase()) ||
    p.description.toLowerCase().includes(searchQuery.toLowerCase())
  );

  const filteredParticipants = participants.filter(u => 
    u.username.toLowerCase().includes(searchQuery.toLowerCase()) ||
    u.role.toLowerCase().includes(searchQuery.toLowerCase())
  );

  const filteredSubmissions = submissions.filter(s => {
    const matchesSearch = s.username.toLowerCase().includes(searchQuery.toLowerCase()) ||
                          s.problem_title.toLowerCase().includes(searchQuery.toLowerCase());
    if (submissionFilter === 'ALL') return matchesSearch;
    if (submissionFilter === 'ACCEPTED') return matchesSearch && s.verdict.includes('ACCEPTED');
    if (submissionFilter === 'REJECTED') return matchesSearch && !s.verdict.includes('ACCEPTED');
    return matchesSearch;
  });

  return (
    <div style={{ display: 'flex', minHeight: '100vh', background: 'var(--bg-primary)' }}>
      
      {/* Toast Notification */}
      {notification && (
        <div style={{
          position: 'fixed',
          top: '20px',
          right: '24px',
          zIndex: 9999,
          background: notification.type === 'success' ? '#065f46' : '#991b1b',
          color: '#fff',
          padding: '12px 20px',
          borderRadius: '8px',
          boxShadow: '0 10px 25px rgba(0,0,0,0.5)',
          display: 'flex',
          alignItems: 'center',
          gap: '10px',
          fontSize: '0.9rem',
          fontWeight: 600,
          animation: 'fadeIn 0.2s ease-in-out'
        }}>
          {notification.type === 'success' ? <CheckCircle2 size={18} /> : <AlertCircle size={18} />}
          {notification.message}
        </div>
      )}

      {/* =================================================================== */}
      {/* ADMIN SIDEBAR                                                       */}
      {/* =================================================================== */}
      <aside style={{
        width: '260px',
        background: 'rgba(17, 24, 39, 0.95)',
        backdropFilter: 'blur(16px)',
        borderRight: '1px solid var(--border-color)',
        display: 'flex',
        flexDirection: 'column',
        position: 'sticky',
        top: 0,
        height: '100vh',
        zIndex: 50
      }}>
        {/* Brand */}
        <div style={{ padding: '24px 20px', borderBottom: '1px solid var(--border-color)', display: 'flex', alignItems: 'center', gap: '12px' }}>
          <div style={{
            background: 'linear-gradient(135deg, #ef4444, #f59e0b)',
            width: '38px',
            height: '38px',
            borderRadius: '10px',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: '#fff',
            boxShadow: '0 4px 12px rgba(239, 68, 68, 0.35)'
          }}>
            <ShieldAlert size={22} />
          </div>
          <div>
            <h2 style={{ fontSize: '1.05rem', fontWeight: 800, color: '#fff', letterSpacing: '-0.02em' }}>
              ADMIN CONSOLE
            </h2>
            <div style={{ fontSize: '0.72rem', color: '#f59e0b', fontWeight: 600 }}>
              Root Privileges Active
            </div>
          </div>
        </div>

        {/* Navigation Items */}
        <nav style={{ padding: '16px 12px', display: 'flex', flexDirection: 'column', gap: '6px', flex: 1 }}>
          {[
            { id: 'dashboard', label: 'Dashboard', icon: LayoutDashboard },
            { id: 'contests', label: 'Contests', icon: Trophy },
            { id: 'problems', label: 'Problems', icon: Code2 },
            { id: 'participants', label: 'Participants', icon: Users },
            { id: 'submissions', label: 'Submissions', icon: FileCode },
          ].map(item => {
            const Icon = item.icon;
            const active = activeTab === item.id;
            return (
              <button
                key={item.id}
                onClick={() => {
                  setActiveTab(item.id);
                  setSearchQuery('');
                }}
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  gap: '12px',
                  padding: '10px 14px',
                  borderRadius: '8px',
                  border: 'none',
                  background: active ? 'rgba(59, 130, 246, 0.2)' : 'transparent',
                  color: active ? '#60a5fa' : 'var(--text-muted)',
                  fontWeight: active ? 700 : 500,
                  fontSize: '0.9rem',
                  cursor: 'pointer',
                  textAlign: 'left',
                  transition: 'all 0.15s ease'
                }}>
                <Icon size={18} color={active ? '#60a5fa' : 'var(--text-dim)'} />
                {item.label}
              </button>
            );
          })}
        </nav>

        {/* Footer Actions */}
        <div style={{ padding: '16px 14px', borderTop: '1px solid var(--border-color)', display: 'flex', flexDirection: 'column', gap: '8px' }}>
          <button
            onClick={onExitAdmin}
            style={{
              background: 'rgba(255, 255, 255, 0.06)',
              color: 'var(--text-main)',
              border: '1px solid var(--border-color)',
              padding: '9px 12px',
              borderRadius: '8px',
              fontSize: '0.85rem',
              fontWeight: 600,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              gap: '8px',
              cursor: 'pointer'
            }}>
            <Code2 size={16} /> Return to Contest
          </button>
          <button
            onClick={onLogout}
            style={{
              background: 'rgba(239, 68, 68, 0.15)',
              color: '#f87171',
              border: '1px solid rgba(239, 68, 68, 0.3)',
              padding: '9px 12px',
              borderRadius: '8px',
              fontSize: '0.85rem',
              fontWeight: 600,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              gap: '8px',
              cursor: 'pointer'
            }}>
            <LogOut size={16} /> Sign Out (Admin)
          </button>
        </div>
      </aside>

      {/* =================================================================== */}
      {/* MAIN ADMIN CONTENT AREA                                             */}
      {/* =================================================================== */}
      <main style={{ flex: 1, padding: '32px', maxWidth: '1440px', overflowY: 'auto' }}>
        
        {/* Top Header */}
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '28px' }}>
          <div>
            <h1 style={{ fontSize: '1.75rem', fontWeight: 800, color: 'var(--text-main)', textTransform: 'capitalize' }}>
              {`${activeTab} Management`}
            </h1>
            <p style={{ color: 'var(--text-muted)', fontSize: '0.88rem', marginTop: '4px' }}>
              Logged in as <strong style={{ color: 'var(--text-main)' }}>{currentUser}</strong> • Real-Time C++ Server Interface
            </p>
          </div>

          <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
            {['contests', 'problems', 'participants', 'submissions'].includes(activeTab) && (
              <div style={{ position: 'relative', width: '260px' }}>
                <Search size={16} style={{ position: 'absolute', left: '12px', top: '10px', color: 'var(--text-dim)' }} />
                <input 
                  type="text" 
                  placeholder={`Search ${activeTab}...`}
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  style={{ width: '100%', paddingLeft: '36px', fontSize: '0.85rem' }}
                />
              </div>
            )}

            {activeTab === 'contests' && (
              <button 
                onClick={() => handleOpenContestModal()}
                style={{
                  background: 'var(--accent-blue)',
                  color: '#fff',
                  border: 'none',
                  padding: '9px 16px',
                  borderRadius: '8px',
                  fontSize: '0.85rem',
                  fontWeight: 600,
                  display: 'flex',
                  alignItems: 'center',
                  gap: '6px',
                  boxShadow: '0 4px 12px rgba(59, 130, 246, 0.3)'
                }}>
                <Plus size={16} /> Create Contest
              </button>
            )}

            {activeTab === 'problems' && (
              <button 
                onClick={() => handleOpenProblemModal()}
                style={{
                  background: 'var(--accent-blue)',
                  color: '#fff',
                  border: 'none',
                  padding: '9px 16px',
                  borderRadius: '8px',
                  fontSize: '0.85rem',
                  fontWeight: 600,
                  display: 'flex',
                  alignItems: 'center',
                  gap: '6px',
                  boxShadow: '0 4px 12px rgba(59, 130, 246, 0.3)'
                }}>
                <Plus size={16} /> Add Problem
              </button>
            )}

            <button
              onClick={() => {
                if (activeTab === 'dashboard') fetchDashboardStats();
                else if (activeTab === 'contests') fetchContests();
                else if (activeTab === 'problems') fetchProblems();
                else if (activeTab === 'participants') fetchParticipants();
                else if (activeTab === 'submissions') fetchSubmissions();
              }}
              style={{
                background: '#FFFFFF',
                color: 'var(--text-main)',
                border: '1px solid var(--border-color)',
                padding: '9px 12px',
                borderRadius: '8px',
                fontSize: '0.85rem',
                display: 'flex',
                alignItems: 'center',
                gap: '6px'
              }}>
              <RefreshCw size={15} className={loading ? 'animate-spin' : ''} /> Refresh
            </button>
          </div>
        </div>

        {/* ----------------------------------------------------------------- */}
        {/* VIEW 1: DASHBOARD METRICS                                         */}
        {/* ----------------------------------------------------------------- */}
        {activeTab === 'dashboard' && stats && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '24px' }}>
            
            {/* Top Stat Cards Grid */}
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(210px, 1fr))', gap: '16px' }}>
              <div className="glass-panel" style={{ padding: '20px' }}>
                <div style={{ color: 'var(--text-muted)', fontSize: '0.8rem', fontWeight: 600 }}>TOTAL PARTICIPANTS</div>
                <div style={{ fontSize: '2.1rem', fontWeight: 800, marginTop: '8px', color: '#60a5fa' }}>{stats.total_participants}</div>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', marginTop: '4px' }}>Registered student accounts</div>
              </div>

              <div className="glass-panel" style={{ padding: '20px' }}>
                <div style={{ color: 'var(--text-muted)', fontSize: '0.8rem', fontWeight: 600 }}>CONTESTS (ACTIVE/TOTAL)</div>
                <div style={{ fontSize: '2.1rem', fontWeight: 800, marginTop: '8px', color: '#34d399' }}>
                  {stats.active_contests} <span style={{ fontSize: '1.2rem', color: 'var(--text-dim)' }}>/ {stats.total_contests}</span>
                </div>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', marginTop: '4px' }}>Active real-time challenges</div>
              </div>

              <div className="glass-panel" style={{ padding: '20px' }}>
                <div style={{ color: 'var(--text-muted)', fontSize: '0.8rem', fontWeight: 600 }}>TOTAL PROBLEMS</div>
                <div style={{ fontSize: '2.1rem', fontWeight: 800, marginTop: '8px', color: '#a78bfa' }}>{stats.total_problems}</div>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', marginTop: '4px' }}>Active problem repository</div>
              </div>

              <div className="glass-panel" style={{ padding: '20px' }}>
                <div style={{ color: 'var(--text-muted)', fontSize: '0.8rem', fontWeight: 600 }}>TOTAL SUBMISSIONS</div>
                <div style={{ fontSize: '2.1rem', fontWeight: 800, marginTop: '8px', color: '#f59e0b' }}>{stats.total_submissions}</div>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', marginTop: '4px' }}>
                  <span style={{ color: '#34d399' }}>{stats.accepted_submissions} AC</span> • <span style={{ color: '#f87171' }}>{stats.rejected_submissions} WA/TLE</span>
                </div>
              </div>
            </div>

            {/* Submissions Breakdown */}
            <div className="glass-panel" style={{ padding: '24px' }}>
              <h3 style={{ fontSize: '1.15rem', fontWeight: 700, marginBottom: '16px', display: 'flex', alignItems: 'center', gap: '8px' }}>
                <Database size={18} color="#315EAA" /> Submission Verdict Breakdown
              </h3>
              <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(280px, 1fr))', gap: '20px' }}>
                <div>
                  <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.85rem', marginBottom: '6px' }}>
                    <span style={{ color: '#15803D', fontWeight: 600 }}>Accepted (AC)</span>
                    <span style={{ fontWeight: 600 }}>{stats.accepted_submissions} ({stats.total_submissions > 0 ? Math.round((stats.accepted_submissions / stats.total_submissions) * 100) : 0}%)</span>
                  </div>
                  <div style={{ width: '100%', height: '8px', background: '#E2E8F0', borderRadius: '4px', overflow: 'hidden' }}>
                    <div style={{ width: `${stats.total_submissions > 0 ? (stats.accepted_submissions / stats.total_submissions) * 100 : 0}%`, height: '100%', background: '#10b981' }}></div>
                  </div>
                </div>

                <div>
                  <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.85rem', marginBottom: '6px' }}>
                    <span style={{ color: '#B91C1C', fontWeight: 600 }}>Rejected / Errors (WA, TLE, RE)</span>
                    <span style={{ fontWeight: 600 }}>{stats.rejected_submissions} ({stats.total_submissions > 0 ? Math.round((stats.rejected_submissions / stats.total_submissions) * 100) : 0}%)</span>
                  </div>
                  <div style={{ width: '100%', height: '8px', background: '#E2E8F0', borderRadius: '4px', overflow: 'hidden' }}>
                    <div style={{ width: `${stats.total_submissions > 0 ? (stats.rejected_submissions / stats.total_submissions) * 100 : 0}%`, height: '100%', background: '#ef4444' }}></div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ----------------------------------------------------------------- */}
        {/* VIEW 2: CONTEST MANAGEMENT                                        */}
        {/* ----------------------------------------------------------------- */}
        {activeTab === 'contests' && (
          <div className="glass-panel" style={{ overflow: 'hidden' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.88rem' }}>
              <thead>
                <tr style={{ borderBottom: '1px solid var(--border-color)', background: '#F8FAFC', color: 'var(--text-muted)' }}>
                  <th style={{ padding: '14px 20px' }}>ID</th>
                  <th style={{ padding: '14px 20px' }}>Contest Name</th>
                  <th style={{ padding: '14px 20px' }}>Status</th>
                  <th style={{ padding: '14px 20px' }}>Duration</th>
                  <th style={{ padding: '14px 20px' }}>Problems</th>
                  <th style={{ padding: '14px 20px' }}>Participants</th>
                  <th style={{ padding: '14px 20px', textAlign: 'right' }}>Actions</th>
                </tr>
              </thead>
              <tbody>
                {filteredContests.length > 0 ? (
                  filteredContests.map(c => (
                    <tr key={c.id} style={{ borderBottom: '1px solid var(--border-color)' }}>
                      <td style={{ padding: '14px 20px', fontFamily: 'var(--font-mono)' }}>#{c.id}</td>
                      <td style={{ padding: '14px 20px' }}>
                        <div style={{ fontWeight: 700, color: 'var(--text-main)' }}>{c.title}</div>
                        <div style={{ fontSize: '0.78rem', color: 'var(--text-dim)', maxWidth: '320px', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                          {c.description}
                        </div>
                      </td>
                      <td style={{ padding: '14px 20px' }}>
                        <span className={`badge ${c.status === 'ACTIVE' ? 'badge-green' : c.status === 'PUBLISHED' ? 'badge-blue' : 'badge-yellow'}`}>
                          {c.status}
                        </span>
                      </td>
                      <td style={{ padding: '14px 20px' }}>{c.duration_minutes} min</td>
                      <td style={{ padding: '14px 20px' }}>{c.problem_count} Problems</td>
                      <td style={{ padding: '14px 20px' }}>{c.participant_count} Registered</td>
                      <td style={{ padding: '14px 20px', textAlign: 'right' }}>
                        <div style={{ display: 'inline-flex', gap: '8px' }}>
                          {c.status !== 'ACTIVE' ? (
                            <button
                              onClick={() => handleUpdateContestStatus(c.id, 'ACTIVE')}
                              style={{ background: 'rgba(16, 185, 129, 0.15)', color: '#34d399', border: '1px solid rgba(16, 185, 129, 0.3)', padding: '5px 10px', borderRadius: '6px', fontSize: '0.75rem', fontWeight: 600 }}>
                              Start
                            </button>
                          ) : (
                            <button
                              onClick={() => handleUpdateContestStatus(c.id, 'STOPPED')}
                              style={{ background: 'rgba(245, 158, 11, 0.15)', color: '#fbbf24', border: '1px solid rgba(245, 158, 11, 0.3)', padding: '5px 10px', borderRadius: '6px', fontSize: '0.75rem', fontWeight: 600 }}>
                              Stop
                            </button>
                          )}
                          <button
                            onClick={() => handleOpenContestModal(c)}
                            style={{ background: 'rgba(59, 130, 246, 0.15)', color: '#60a5fa', border: '1px solid rgba(59, 130, 246, 0.3)', padding: '5px 8px', borderRadius: '6px' }}>
                            <Edit2 size={13} />
                          </button>
                          <button
                            onClick={() => setDeleteConfirm({ type: 'contest', id: c.id, name: c.title })}
                            style={{ background: 'rgba(239, 68, 68, 0.15)', color: '#f87171', border: '1px solid rgba(239, 68, 68, 0.3)', padding: '5px 8px', borderRadius: '6px' }}>
                            <Trash2 size={13} />
                          </button>
                        </div>
                      </td>
                    </tr>
                  ))
                ) : (
                  <tr>
                    <td colSpan="7" style={{ textAlign: 'center', padding: '36px', color: 'var(--text-dim)' }}>
                      No contests found matching criteria.
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        )}

        {/* ----------------------------------------------------------------- */}
        {/* VIEW 3: PROBLEM MANAGEMENT                                        */}
        {/* ----------------------------------------------------------------- */}
        {activeTab === 'problems' && (
          <div className="glass-panel" style={{ overflow: 'hidden' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.88rem' }}>
              <thead>
                <tr style={{ borderBottom: '1px solid var(--border-color)', background: '#F8FAFC', color: 'var(--text-muted)' }}>
                  <th style={{ padding: '14px 20px' }}>ID</th>
                  <th style={{ padding: '14px 20px' }}>Title & Description</th>
                  <th style={{ padding: '14px 20px' }}>Difficulty</th>
                  <th style={{ padding: '14px 20px' }}>Points</th>
                  <th style={{ padding: '14px 20px' }}>Time Limit</th>
                  <th style={{ padding: '14px 20px' }}>Memory</th>
                  <th style={{ padding: '14px 20px', textAlign: 'right' }}>Actions</th>
                </tr>
              </thead>
              <tbody>
                {filteredProblems.length > 0 ? (
                  filteredProblems.map(p => (
                    <tr key={p.id} style={{ borderBottom: '1px solid var(--border-color)' }}>
                      <td style={{ padding: '14px 20px', fontFamily: 'var(--font-mono)' }}>#{p.id}</td>
                      <td style={{ padding: '14px 20px' }}>
                        <div style={{ fontWeight: 700, color: 'var(--text-main)' }}>{p.title}</div>
                        <div style={{ fontSize: '0.78rem', color: 'var(--text-dim)', maxWidth: '340px', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                          {p.description}
                        </div>
                      </td>
                      <td style={{ padding: '14px 20px' }}>
                        <span className={`badge ${p.difficulty === 'Easy' ? 'badge-green' : p.difficulty === 'Hard' ? 'badge-red' : 'badge-yellow'}`}>
                          {p.difficulty || 'Medium'}
                        </span>
                      </td>
                      <td style={{ padding: '14px 20px', fontWeight: 600, color: '#34d399' }}>{p.points} pts</td>
                      <td style={{ padding: '14px 20px' }}>{p.time_limit_sec}s</td>
                      <td style={{ padding: '14px 20px' }}>{p.memory_limit_mb} MB</td>
                      <td style={{ padding: '14px 20px', textAlign: 'right' }}>
                        <div style={{ display: 'inline-flex', gap: '8px' }}>
                          <button
                            onClick={() => handleOpenProblemModal(p)}
                            style={{ background: 'rgba(59, 130, 246, 0.15)', color: '#60a5fa', border: '1px solid rgba(59, 130, 246, 0.3)', padding: '5px 8px', borderRadius: '6px' }}>
                            <Edit2 size={13} />
                          </button>
                          <button
                            onClick={() => setDeleteConfirm({ type: 'problem', id: p.id, name: p.title })}
                            style={{ background: 'rgba(239, 68, 68, 0.15)', color: '#f87171', border: '1px solid rgba(239, 68, 68, 0.3)', padding: '5px 8px', borderRadius: '6px' }}>
                            <Trash2 size={13} />
                          </button>
                        </div>
                      </td>
                    </tr>
                  ))
                ) : (
                  <tr>
                    <td colSpan="7" style={{ textAlign: 'center', padding: '36px', color: 'var(--text-dim)' }}>
                      No problems found matching criteria.
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        )}

        {/* ----------------------------------------------------------------- */}
        {/* VIEW 4: PARTICIPANT MANAGEMENT                                    */}
        {/* ----------------------------------------------------------------- */}
        {activeTab === 'participants' && (
          <div className="glass-panel" style={{ overflow: 'hidden' }}>
            <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.88rem' }}>
              <thead>
                <tr style={{ borderBottom: '1px solid var(--border-color)', background: '#F8FAFC', color: 'var(--text-muted)' }}>
                  <th style={{ padding: '14px 20px' }}>Username</th>
                  <th style={{ padding: '14px 20px' }}>Role</th>
                  <th style={{ padding: '14px 20px' }}>Status</th>
                  <th style={{ padding: '14px 20px' }}>Score</th>
                  <th style={{ padding: '14px 20px' }}>Solved Count</th>
                  <th style={{ padding: '14px 20px' }}>Total Submissions</th>
                  <th style={{ padding: '14px 20px', textAlign: 'right' }}>Actions</th>
                </tr>
              </thead>
              <tbody>
                {filteredParticipants.length > 0 ? (
                  filteredParticipants.map(u => (
                    <tr key={u.username} style={{ borderBottom: '1px solid var(--border-color)' }}>
                      <td style={{ padding: '14px 20px', fontWeight: 700, color: 'var(--text-main)' }}>{u.username}</td>
                      <td style={{ padding: '14px 20px' }}>
                        <span className={`badge ${u.role === 'admin' ? 'badge-purple' : 'badge-blue'}`}>
                          {u.role.toUpperCase()}
                        </span>
                      </td>
                      <td style={{ padding: '14px 20px' }}>
                        {u.is_suspended ? (
                          <span className="badge badge-red">SUSPENDED</span>
                        ) : (
                          <span className="badge badge-green">ACTIVE</span>
                        )}
                      </td>
                      <td style={{ padding: '14px 20px', color: '#34d399', fontWeight: 600 }}>{u.total_score} pts</td>
                      <td style={{ padding: '14px 20px' }}>{u.solved_problems_count}</td>
                      <td style={{ padding: '14px 20px' }}>{u.total_submissions}</td>
                      <td style={{ padding: '14px 20px', textAlign: 'right' }}>
                        {u.role !== 'admin' ? (
                          <button
                            onClick={() => handleToggleSuspendUser(u.username, u.is_suspended)}
                            style={{
                              background: u.is_suspended ? 'rgba(16, 185, 129, 0.15)' : 'rgba(239, 68, 68, 0.15)',
                              color: u.is_suspended ? '#34d399' : '#f87171',
                              border: `1px solid ${u.is_suspended ? 'rgba(16, 185, 129, 0.3)' : 'rgba(239, 68, 68, 0.3)'}`,
                              padding: '6px 12px',
                              borderRadius: '6px',
                              fontSize: '0.78rem',
                              fontWeight: 600,
                              cursor: 'pointer'
                            }}>
                            {u.is_suspended ? 'Reactivate' : 'Suspend Account'}
                          </button>
                        ) : (
                          <span style={{ fontSize: '0.75rem', color: 'var(--text-dim)' }}>Protected</span>
                        )}
                      </td>
                    </tr>
                  ))
                ) : (
                  <tr>
                    <td colSpan="7" style={{ textAlign: 'center', padding: '36px', color: 'var(--text-dim)' }}>
                      No participants found.
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        )}

        {/* ----------------------------------------------------------------- */}
        {/* VIEW 5: SUBMISSIONS AUDIT                                         */}
        {/* ----------------------------------------------------------------- */}
        {activeTab === 'submissions' && (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
            
            {/* Filter Toggle Buttons */}
            <div style={{ display: 'flex', gap: '8px' }}>
              {['ALL', 'ACCEPTED', 'REJECTED'].map(f => (
                <button
                  key={f}
                  onClick={() => setSubmissionFilter(f)}
                  style={{
                    background: submissionFilter === f ? 'var(--accent-blue)' : '#FFFFFF',
                    color: submissionFilter === f ? '#fff' : 'var(--text-muted)',
                    border: '1px solid var(--border-color)',
                    padding: '6px 14px',
                    borderRadius: '6px',
                    fontSize: '0.8rem',
                    fontWeight: 600,
                    cursor: 'pointer'
                  }}>
                  {f} Submissions
                </button>
              ))}
            </div>

            <div className="glass-panel" style={{ overflow: 'hidden' }}>
              <table style={{ width: '100%', borderCollapse: 'collapse', textAlign: 'left', fontSize: '0.88rem' }}>
                <thead>
                  <tr style={{ borderBottom: '1px solid var(--border-color)', background: '#F8FAFC', color: 'var(--text-muted)' }}>
                    <th style={{ padding: '14px 20px' }}>#ID</th>
                    <th style={{ padding: '14px 20px' }}>User</th>
                    <th style={{ padding: '14px 20px' }}>Problem</th>
                    <th style={{ padding: '14px 20px' }}>Verdict</th>
                    <th style={{ padding: '14px 20px' }}>Runtime</th>
                    <th style={{ padding: '14px 20px' }}>Memory</th>
                    <th style={{ padding: '14px 20px' }}>Timestamp</th>
                    <th style={{ padding: '14px 20px', textAlign: 'right' }}>Inspection</th>
                  </tr>
                </thead>
                <tbody>
                  {filteredSubmissions.length > 0 ? (
                    filteredSubmissions.map(s => (
                      <tr key={s.id} style={{ borderBottom: '1px solid var(--border-color)' }}>
                        <td style={{ padding: '14px 20px', fontFamily: 'var(--font-mono)' }}>#{s.id}</td>
                        <td style={{ padding: '14px 20px', fontWeight: 600, color: 'var(--text-main)' }}>{s.username}</td>
                        <td style={{ padding: '14px 20px' }}>#{s.problem_id}: {s.problem_title}</td>
                        <td style={{ padding: '14px 20px' }}>
                          <span className={`badge ${s.verdict.includes('ACCEPTED') ? 'badge-green' : 'badge-red'}`}>
                            {s.verdict}
                          </span>
                        </td>
                        <td style={{ padding: '14px 20px' }}>{s.execution_time_ms ? s.execution_time_ms.toFixed(1) : '0.0'}ms</td>
                        <td style={{ padding: '14px 20px' }}>{s.memory_used_kb} KB</td>
                        <td style={{ padding: '14px 20px', color: 'var(--text-dim)', fontSize: '0.8rem' }}>
                          {new Date(s.timestamp * 1000).toLocaleTimeString()}
                        </td>
                        <td style={{ padding: '14px 20px', textAlign: 'right' }}>
                          <button
                            onClick={() => setViewingCodeModal(s)}
                            style={{
                              background: 'rgba(59, 130, 246, 0.15)',
                              color: '#60a5fa',
                              border: '1px solid rgba(59, 130, 246, 0.3)',
                              padding: '5px 10px',
                              borderRadius: '6px',
                              fontSize: '0.78rem',
                              fontWeight: 600,
                              cursor: 'pointer',
                              display: 'inline-flex',
                              alignItems: 'center',
                              gap: '4px'
                            }}>
                            <Eye size={13} /> View Code
                          </button>
                        </td>
                      </tr>
                    ))
                  ) : (
                    <tr>
                      <td colSpan="8" style={{ textAlign: 'center', padding: '36px', color: 'var(--text-dim)' }}>
                        No submissions recorded matching this filter.
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
          </div>
        )}

      </main>

      {/* =================================================================== */}
      {/* MODAL: CONTEST CREATE / EDIT                                        */}
      {/* =================================================================== */}
      {isContestModalOpen && (
        <div style={{
          position: 'fixed',
          inset: 0,
          background: 'rgba(0,0,0,0.7)',
          backdropFilter: 'blur(6px)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 1000
        }}>
          <div className="glass-panel" style={{ width: '520px', padding: '28px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '20px' }}>
              <h3 style={{ fontSize: '1.25rem', fontWeight: 700, color: 'var(--text-main)' }}>
                {editingContest ? 'Edit Contest' : 'Create New Contest'}
              </h3>
              <button onClick={() => setIsContestModalOpen(false)} style={{ background: 'transparent', border: 'none', color: 'var(--text-dim)', cursor: 'pointer' }}>
                <X size={18} />
              </button>
            </div>

            <form onSubmit={handleSaveContest} style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
              <div>
                <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Contest Title</label>
                <input 
                  type="text" 
                  value={contestForm.title} 
                  onChange={e => setContestForm({ ...contestForm, title: e.target.value })} 
                  placeholder="e.g. Operating Systems Lab Championship" 
                  style={{ width: '100%' }}
                />
              </div>

              <div>
                <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Description</label>
                <textarea 
                  value={contestForm.description} 
                  onChange={e => setContestForm({ ...contestForm, description: e.target.value })} 
                  placeholder="Detailed contest guidelines and syllabus topics..." 
                  style={{ width: '100%', height: '80px' }}
                />
              </div>

              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '14px' }}>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Duration (Minutes)</label>
                  <input 
                    type="number" 
                    value={contestForm.duration_minutes} 
                    onChange={e => setContestForm({ ...contestForm, duration_minutes: parseInt(e.target.value) || 60 })} 
                    style={{ width: '100%' }}
                  />
                </div>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Status</label>
                  <select 
                    value={contestForm.status} 
                    onChange={e => setContestForm({ ...contestForm, status: e.target.value })} 
                    style={{ width: '100%' }}>
                    <option value="DRAFT">DRAFT</option>
                    <option value="ACTIVE">ACTIVE</option>
                    <option value="STOPPED">STOPPED</option>
                    <option value="PUBLISHED">PUBLISHED</option>
                  </select>
                </div>
              </div>

              <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '10px', marginTop: '12px' }}>
                <button type="button" onClick={() => setIsContestModalOpen(false)} style={{ background: 'transparent', border: '1px solid var(--border-color)', color: 'var(--text-muted)', padding: '8px 16px', borderRadius: '6px' }}>
                  Cancel
                </button>
                <button type="submit" style={{ background: 'var(--accent-blue)', color: '#fff', border: 'none', padding: '8px 20px', borderRadius: '6px', fontWeight: 600 }}>
                  Save Contest
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =================================================================== */}
      {/* MODAL: PROBLEM CREATE / EDIT                                        */}
      {/* =================================================================== */}
      {isProblemModalOpen && (
        <div style={{
          position: 'fixed',
          inset: 0,
          background: 'rgba(0,0,0,0.7)',
          backdropFilter: 'blur(6px)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 1000
        }}>
          <div className="glass-panel" style={{ width: '680px', maxHeight: '90vh', overflowY: 'auto', padding: '28px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '20px' }}>
              <h3 style={{ fontSize: '1.25rem', fontWeight: 700, color: 'var(--text-main)' }}>
                {editingProblem ? `Edit Problem #${editingProblem.id}` : 'Add New Problem'}
              </h3>
              <button onClick={() => setIsProblemModalOpen(false)} style={{ background: 'transparent', border: 'none', color: 'var(--text-dim)', cursor: 'pointer' }}>
                <X size={18} />
              </button>
            </div>

            <form onSubmit={handleSaveProblem} style={{ display: 'flex', flexDirection: 'column', gap: '16px' }}>
              <div style={{ display: 'grid', gridTemplateColumns: '2fr 1fr', gap: '14px' }}>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Problem Title</label>
                  <input 
                    type="text" 
                    value={problemForm.title} 
                    onChange={e => setProblemForm({ ...problemForm, title: e.target.value })} 
                    placeholder="e.g. Memory Page Replacement Simulation" 
                    style={{ width: '100%' }}
                  />
                </div>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Difficulty</label>
                  <select 
                    value={problemForm.difficulty} 
                    onChange={e => setProblemForm({ ...problemForm, difficulty: e.target.value })} 
                    style={{ width: '100%' }}>
                    <option value="Easy">Easy</option>
                    <option value="Medium">Medium</option>
                    <option value="Hard">Hard</option>
                  </select>
                </div>
              </div>

              <div>
                <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Problem Description</label>
                <textarea 
                  value={problemForm.description} 
                  onChange={e => setProblemForm({ ...problemForm, description: e.target.value })} 
                  placeholder="Problem narrative and explanation..." 
                  style={{ width: '100%', height: '70px' }}
                />
              </div>

              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: '12px' }}>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Points</label>
                  <input 
                    type="number" 
                    value={problemForm.points} 
                    onChange={e => setProblemForm({ ...problemForm, points: parseInt(e.target.value) || 100 })} 
                    style={{ width: '100%' }}
                  />
                </div>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Time Limit (s)</label>
                  <input 
                    type="number" 
                    step="0.5" 
                    value={problemForm.time_limit_sec} 
                    onChange={e => setProblemForm({ ...problemForm, time_limit_sec: parseFloat(e.target.value) || 3.0 })} 
                    style={{ width: '100%' }}
                  />
                </div>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Memory (MB)</label>
                  <input 
                    type="number" 
                    value={problemForm.memory_limit_mb} 
                    onChange={e => setProblemForm({ ...problemForm, memory_limit_mb: parseInt(e.target.value) || 64 })} 
                    style={{ width: '100%' }}
                  />
                </div>
              </div>

              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '14px' }}>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Input Format</label>
                  <input 
                    type="text" 
                    value={problemForm.input_format} 
                    onChange={e => setProblemForm({ ...problemForm, input_format: e.target.value })} 
                    placeholder="e.g. First line contains N" 
                    style={{ width: '100%' }}
                  />
                </div>
                <div>
                  <label style={{ display: 'block', fontSize: '0.82rem', fontWeight: 600, color: 'var(--text-muted)', marginBottom: '6px' }}>Constraints</label>
                  <input 
                    type="text" 
                    value={problemForm.constraints} 
                    onChange={e => setProblemForm({ ...problemForm, constraints: e.target.value })} 
                    placeholder="e.g. 1 <= N <= 10^5" 
                    style={{ width: '100%' }}
                  />
                </div>
              </div>

              {/* Sample Test Cases (Publicly Visible) */}
              <div style={{ borderTop: '1px solid var(--border-color)', paddingTop: '12px' }}>
                <span style={{ fontSize: '0.85rem', fontWeight: 700, color: '#60a5fa' }}>Sample Test Case (Public to Participants)</span>
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '14px', marginTop: '8px' }}>
                  <div>
                    <label style={{ display: 'block', fontSize: '0.78rem', color: 'var(--text-dim)', marginBottom: '4px' }}>Sample Input</label>
                    <textarea 
                      value={problemForm.sample_input} 
                      onChange={e => setProblemForm({ ...problemForm, sample_input: e.target.value })} 
                      className="mono-font" 
                      style={{ width: '100%', height: '60px' }}
                    />
                  </div>
                  <div>
                    <label style={{ display: 'block', fontSize: '0.78rem', color: 'var(--text-dim)', marginBottom: '4px' }}>Sample Output</label>
                    <textarea 
                      value={problemForm.sample_output} 
                      onChange={e => setProblemForm({ ...problemForm, sample_output: e.target.value })} 
                      className="mono-font" 
                      style={{ width: '100%', height: '60px' }}
                    />
                  </div>
                </div>
              </div>

              {/* Hidden Test Cases (Evaluated by Sandbox, Never exposed to public API) */}
              <div style={{ borderTop: '1px solid var(--border-color)', paddingTop: '12px' }}>
                <span style={{ fontSize: '0.85rem', fontWeight: 700, color: '#f59e0b' }}>Hidden Test Case (Sandbox Judge Only — Secret)</span>
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '14px', marginTop: '8px' }}>
                  <div>
                    <label style={{ display: 'block', fontSize: '0.78rem', color: 'var(--text-dim)', marginBottom: '4px' }}>Hidden Input</label>
                    <textarea 
                      value={problemForm.hidden_input} 
                      onChange={e => setProblemForm({ ...problemForm, hidden_input: e.target.value })} 
                      className="mono-font" 
                      style={{ width: '100%', height: '60px' }}
                    />
                  </div>
                  <div>
                    <label style={{ display: 'block', fontSize: '0.78rem', color: 'var(--text-dim)', marginBottom: '4px' }}>Hidden Output</label>
                    <textarea 
                      value={problemForm.hidden_output} 
                      onChange={e => setProblemForm({ ...problemForm, hidden_output: e.target.value })} 
                      className="mono-font" 
                      style={{ width: '100%', height: '60px' }}
                    />
                  </div>
                </div>
              </div>

              <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '10px', marginTop: '12px' }}>
                <button type="button" onClick={() => setIsProblemModalOpen(false)} style={{ background: 'transparent', border: '1px solid var(--border-color)', color: 'var(--text-muted)', padding: '8px 16px', borderRadius: '6px' }}>
                  Cancel
                </button>
                <button type="submit" style={{ background: 'var(--accent-blue)', color: '#fff', border: 'none', padding: '8px 20px', borderRadius: '6px', fontWeight: 600 }}>
                  Save Problem
                </button>
              </div>
            </form>
          </div>
        </div>
      )}

      {/* =================================================================== */}
      {/* MODAL: DELETE CONFIRMATION                                          */}
      {/* =================================================================== */}
      {deleteConfirm && (
        <div style={{
          position: 'fixed',
          inset: 0,
          background: 'rgba(0,0,0,0.7)',
          backdropFilter: 'blur(6px)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 1000
        }}>
          <div className="glass-panel" style={{ width: '420px', padding: '24px' }}>
            <h3 style={{ fontSize: '1.2rem', fontWeight: 700, color: '#f87171', display: 'flex', alignItems: 'center', gap: '8px' }}>
              <AlertCircle size={20} /> Confirm Deletion
            </h3>
            <p style={{ color: 'var(--text-muted)', fontSize: '0.9rem', lineHeight: 1.5, margin: '14px 0 20px' }}>
              Are you sure you want to permanently delete the {deleteConfirm.type} <strong style={{ color: 'var(--text-main)' }}>"{deleteConfirm.name}"</strong>? This action cannot be undone.
            </p>
            <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '10px' }}>
              <button 
                onClick={() => setDeleteConfirm(null)}
                style={{ background: 'transparent', border: '1px solid var(--border-color)', color: 'var(--text-muted)', padding: '8px 16px', borderRadius: '6px' }}>
                Cancel
              </button>
              <button 
                onClick={() => {
                  if (deleteConfirm.type === 'contest') handleDeleteContest(deleteConfirm.id);
                  else if (deleteConfirm.type === 'problem') handleDeleteProblem(deleteConfirm.id);
                }}
                style={{ background: '#dc2626', color: '#fff', border: 'none', padding: '8px 18px', borderRadius: '6px', fontWeight: 600 }}>
                Yes, Delete
              </button>
            </div>
          </div>
        </div>
      )}

      {/* =================================================================== */}
      {/* MODAL: SUBMISSION CODE INSPECTION                                   */}
      {/* =================================================================== */}
      {viewingCodeModal && (
        <div style={{
          position: 'fixed',
          inset: 0,
          background: 'rgba(0,0,0,0.75)',
          backdropFilter: 'blur(6px)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          zIndex: 1000
        }}>
          <div className="glass-panel" style={{ width: '700px', maxHeight: '85vh', overflowY: 'auto', padding: '28px' }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
              <div>
                <h3 style={{ fontSize: '1.2rem', fontWeight: 700, color: 'var(--text-main)' }}>
                  Submission #{viewingCodeModal.id} by {viewingCodeModal.username}
                </h3>
                <div style={{ fontSize: '0.8rem', color: 'var(--text-dim)' }}>
                  Problem #{viewingCodeModal.problem_id}: {viewingCodeModal.problem_title} • {viewingCodeModal.verdict}
                </div>
              </div>
              <button onClick={() => setViewingCodeModal(null)} style={{ background: 'transparent', border: 'none', color: 'var(--text-dim)', cursor: 'pointer' }}>
                <X size={18} />
              </button>
            </div>

            <div style={{ display: 'flex', gap: '16px', fontSize: '0.82rem', marginBottom: '16px' }}>
              <div>Runtime: <strong style={{ color: 'var(--text-main)' }}>{viewingCodeModal.execution_time_ms} ms</strong></div>
              <div>Memory: <strong style={{ color: 'var(--text-main)' }}>{viewingCodeModal.memory_used_kb} KB</strong></div>
              <div>Timestamp: <strong style={{ color: 'var(--text-main)' }}>{new Date(viewingCodeModal.timestamp * 1000).toLocaleString()}</strong></div>
            </div>

            <pre style={{
              background: '#0d1117',
              padding: '16px',
              borderRadius: '8px',
              fontFamily: 'var(--font-mono)',
              fontSize: '0.85rem',
              color: '#c9d1d9',
              whiteSpace: 'pre-wrap',
              maxHeight: '340px',
              overflowY: 'auto',
              border: '1px solid var(--border-color)'
            }}>
              {viewingCodeModal.code}
            </pre>

            {viewingCodeModal.compiler_output && (
              <div style={{ marginTop: '14px', background: 'rgba(239, 68, 68, 0.1)', padding: '12px', borderRadius: '6px', border: '1px solid rgba(239,68,68,0.25)' }}>
                <div style={{ color: '#f87171', fontSize: '0.8rem', fontWeight: 700, marginBottom: '4px' }}>Compiler Diagnostics:</div>
                <pre style={{ color: '#fca5a5', fontSize: '0.8rem', fontFamily: 'var(--font-mono)', margin: 0 }}>
                  {viewingCodeModal.compiler_output}
                </pre>
              </div>
            )}
          </div>
        </div>
      )}

    </div>
  );
}
