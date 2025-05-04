-- Eliminar la base de datos si existe para empezar de cero
-- DROP DATABASE IF EXISTS Constructora;

-- Crear la base de datos
CREATE DATABASE Constructora;

-- Conectar a la base de datos recién creada
\c constructora

-- 1. Tabla de empresas solicitantes
CREATE TABLE empresas (
    id SERIAL PRIMARY KEY,
    nombre             VARCHAR(100) NOT NULL,
    direccion          VARCHAR(150) NOT NULL,
    telefono           VARCHAR(20)  NOT NULL,
    correo             VARCHAR(100) NOT NULL,
    rfc                VARCHAR(20)  NOT NULL,
    contacto_encargado VARCHAR(100) NOT NULL
);

-- 2. Tabla de supervisores
CREATE TABLE supervisores (
    id SERIAL PRIMARY KEY,
    nombre    VARCHAR(50)  NOT NULL,
    apellidos VARCHAR(50)  NOT NULL,
    telefono  VARCHAR(20)  NOT NULL,
    correo    VARCHAR(100) NOT NULL  
);

-- 3. Tabla de solicitudes de proyecto
CREATE TABLE solicitud_proyecto (
    id                SERIAL        PRIMARY KEY,
    empresa_id        INTEGER       NOT NULL REFERENCES empresas(id),
    fecha_solicitud   DATE          NOT NULL,
    presupuesto       NUMERIC(12,2) NOT NULL,
    anticipo          NUMERIC(12,2) NOT NULL,
    folio             VARCHAR(50)   NOT NULL UNIQUE, 
    estado            VARCHAR(15)   NOT NULL,        
    razon_cancelacion TEXT,                          

    CONSTRAINT chk_estado_solicitud CHECK (estado IN ('APERTURADO', 'ACEPTADO', 'CANCELADO'))
);

-- 4. Tabla de proyectos aceptados
CREATE TABLE proyecto_aceptado (
    id                 SERIAL        PRIMARY KEY,
    solicitud_id       INTEGER       NOT NULL REFERENCES solicitud_proyecto(id),
    nombre_proyecto    VARCHAR(100)  NOT NULL,
    fecha_inicio       DATE          NOT NULL,
    fecha_fin          DATE          NOT NULL, 
    monto              NUMERIC(12,2) NOT NULL, 
    estatus            VARCHAR(15)   NOT NULL,
    porcentaje_avance  NUMERIC(5,2)  NOT NULL, 
    ubicacion          VARCHAR(100)  NOT NULL,
    descripcion        VARCHAR(200),
    prioridad          VARCHAR(10)   NOT NULL, 
    id_supervisor      INTEGER       NOT NULL REFERENCES supervisores(id),

    CONSTRAINT chk_estado_proyecto CHECK (estatus IN ('EN_PROCESO', 'TERMINADO')),

    CONSTRAINT chk_prioridad CHECK (prioridad IN ('ROJO', 'NARANJA', 'AMARILLO')),

    CONSTRAINT chk_porcentaje CHECK (porcentaje_avance >= 0 AND porcentaje_avance <= 100)
);

-- 5. Creación de Índices para mejorar el rendimiento de las consultas/reportes

-- Índice para buscar solicitudes por fecha, estado o empresa
CREATE INDEX idx_solicitud_fecha ON solicitud_proyecto (fecha_solicitud);
CREATE INDEX idx_solicitud_estado ON solicitud_proyecto (estado);
CREATE INDEX idx_solicitud_empresa ON solicitud_proyecto (empresa_id);

-- Índice para buscar proyectos por fechas, estado, avance, prioridad o supervisor
CREATE INDEX idx_proyecto_fechas ON proyecto_aceptado (fecha_inicio, fecha_fin);
CREATE INDEX idx_proyecto_estatus ON proyecto_aceptado (estatus);
CREATE INDEX idx_proyecto_avance ON proyecto_aceptado (porcentaje_avance);
CREATE INDEX idx_proyecto_prioridad ON proyecto_aceptado (prioridad);
CREATE INDEX idx_proyecto_supervisor ON proyecto_aceptado (id_supervisor);

-- Índice para buscar empresas por nombre
CREATE INDEX idx_empresa_nombre ON empresas (nombre);
