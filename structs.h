typedef struct {
    int id;
    char nombre[100]; 
    char direccion[150];
    char telefono[20];
    char correo[100];
    char rfc[20];
    char contacto_encargado[100];
} Empresa;

typedef struct {
    int id;
    char nombre[50];
    char apellidos[50];
    char telefono[20];
    char correo[100];
} Supervisor;

typedef struct {
    int id;
    int empresa_id;
    char fecha_solicitud[12]; //formato YYYY-MM-DD
    double presupuesto;      
    double anticipo;
    char folio[50];
    char estado[15];        //APERTURADO', 'ACEPTADO', 'CANCELADO'
    char razon_cancelacion[255];
} SolicitudProyecto;

typedef struct {
    int id;
    int solicitud_id;
    char nombre_proyecto[100];
    char fecha_inicio[12];
    char fecha_fin[12];
    double monto;
    char estatus[20];       //'EN_PROCESO', 'TERMINADO'
    double porcentaje_avance;
    char ubicacion[100];
    char descripcion[200];
    char prioridad[15];     //'ROJO', 'NARANJA', 'AMARILLO'
    int id_supervisor;
} ProyectoAceptado;