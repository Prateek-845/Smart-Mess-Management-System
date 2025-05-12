from flask import Flask, render_template, request, jsonify, redirect, url_for, session
import sqlite3
from datetime import datetime, timedelta
import random
from functools import wraps

app = Flask(__name__)
app.secret_key = 'your-secret-key-here'  # Replace with a strong secret key

def init_db():
    conn = sqlite3.connect('data.db')
    c = conn.cursor()
    c.execute("""CREATE TABLE IF NOT EXISTS sensor_data
                 (id INTEGER PRIMARY KEY AUTOINCREMENT,
                  timestamp DATETIME,
                  occupancy INTEGER,
                  total_visitors INTEGER,
                  temperature REAL,
                  humidity REAL,
                  distance REAL,
                  full_count INTEGER,
                  fan_speed TEXT)""")
    conn.commit()
    conn.close()

init_db()

def get_sample_data():
    return {
        'occupancy': random.randint(0, 15),
        'total_visitors': random.randint(0, 500),
        'temperature': round(random.uniform(20.0, 35.0), 1),
        'humidity': round(random.uniform(30.0, 80.0), 1),
        'distance': round(random.uniform(5.0, 30.0), 1),
        'full_count': random.randint(0, 20),
        'fan_speed': random.choice(['Low', 'Medium', 'High']),
        'is_live': False
    }

def get_current_data():
    conn = sqlite3.connect('data.db')
    c = conn.cursor()
    c.execute('SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 1')
    latest = c.fetchone()
    conn.close()
    if latest:
        try:
            last_time = datetime.strptime(latest[1], '%Y-%m-%d %H:%M:%S.%f')
        except:
            last_time = datetime.now() - timedelta(minutes=10)
        if datetime.now() - last_time < timedelta(minutes=2):
            return {
                'occupancy': latest[2],
                'total_visitors': latest[3],
                'temperature': latest[4],
                'humidity': latest[5],
                'distance': latest[6],
                'full_count': latest[7],
                'fan_speed': latest[8],
                'is_live': True
            }
    return get_sample_data()

@app.context_processor
def inject_current_data():
    return {'current_occupancy': get_current_data()['occupancy']}

def login_required(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        if not session.get('logged_in'):
            return redirect(url_for('login', next=request.url))
        return func(*args, **kwargs)
    return wrapper

@app.route('/')
def index():
    data = get_current_data()
    return render_template('index.html', data=data)

@app.route('/api/current')
def api_current():
    return jsonify(get_current_data())

@app.route('/api/data', methods=['POST'])
def handle_data():
    data = request.get_json()
    conn = sqlite3.connect('data.db')
    c = conn.cursor()
    c.execute("""INSERT INTO sensor_data 
                 (timestamp, occupancy, total_visitors, temperature, humidity, distance, full_count, fan_speed)
                 VALUES (?, ?, ?, ?, ?, ?, ?, ?)""",
              (datetime.now(), data['occupancy'], data['total_visitors'],
               data['temperature'], data['humidity'], data['distance'],
               data['full_count'], data['fan_speed']))
    conn.commit()
    conn.close()
    return jsonify({"status": "success"}), 200

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        password = request.form.get('password')
        if password == '123':
            session['logged_in'] = True
            next_url = request.args.get('next') or url_for('weekly')
            return redirect(next_url)
        else:
            return render_template('login.html', error='Invalid password')
    return render_template('login.html')

@app.route('/logout', methods=['GET', 'POST'])
def logout():
    session.clear()  # Completely clear the session
    return redirect(url_for('index'))

@app.route('/weekly')
@login_required
def weekly():
    conn = sqlite3.connect('data.db')
    c = conn.cursor()
    c.execute('SELECT * FROM sensor_data ORDER BY timestamp DESC')
    data = c.fetchall()
    conn.close()
    return render_template('weekly.html', data=data)

@app.route('/save_live_data', methods=['POST'])
@login_required
def save_live_data():
    data = get_current_data()
    conn = sqlite3.connect('data.db')
    c = conn.cursor()
    c.execute("""INSERT INTO sensor_data 
                 (timestamp, occupancy, total_visitors, temperature, humidity, distance, full_count, fan_speed)
                 VALUES (?, ?, ?, ?, ?, ?, ?, ?)""",
              (datetime.now(), data['occupancy'], data['total_visitors'],
               data['temperature'], data['humidity'], data['distance'],
               data['full_count'], data['fan_speed']))
    conn.commit()
    conn.close()
    return redirect(url_for('weekly'))

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True)
