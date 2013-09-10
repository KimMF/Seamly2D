#include "vmodelingspline.h"
#include <QMenu>
#include <QDebug>
#include "geometry/vspline.h"


VModelingSpline::VModelingSpline(VDomDocument *doc, VContainer *data, qint64 id,
                         Tool::Enum typeCreation,
                         QGraphicsItem *parent):VModelingTool(doc, data, id), QGraphicsPathItem(parent),
    dialogSpline(QSharedPointer<DialogSpline>()), controlPoints(QVector<VControlPointSpline *>()){

    VSpline spl = data->GetModelingSpline(id);
    QPainterPath path;
    path.addPath(spl.GetPath());
    path.setFillRule( Qt::WindingFill );
    this->setPath(path);
    this->setPen(QPen(Qt::black, widthHairLine));
    this->setFlag(QGraphicsItem::ItemIsSelectable, true);
    this->setAcceptHoverEvents(true);

    VControlPointSpline *controlPoint1 = new VControlPointSpline(1, SplinePoint::FirstPoint, spl.GetP2(),
                                                                 spl.GetPointP1().toQPointF(), this);
    connect(controlPoint1, &VControlPointSpline::ControlPointChangePosition, this,
            &VModelingSpline::ControlPointChangePosition);
    connect(this, &VModelingSpline::RefreshLine, controlPoint1, &VControlPointSpline::RefreshLine);
    connect(this, &VModelingSpline::setEnabledPoint, controlPoint1, &VControlPointSpline::setEnabledPoint);
    controlPoints.append(controlPoint1);

    VControlPointSpline *controlPoint2 = new VControlPointSpline(1, SplinePoint::LastPoint, spl.GetP3(),
                                                                 spl.GetPointP4().toQPointF(), this);
    connect(controlPoint2, &VControlPointSpline::ControlPointChangePosition, this,
            &VModelingSpline::ControlPointChangePosition);
    connect(this, &VModelingSpline::RefreshLine, controlPoint2, &VControlPointSpline::RefreshLine);
    connect(this, &VModelingSpline::setEnabledPoint, controlPoint2, &VControlPointSpline::setEnabledPoint);
    controlPoints.append(controlPoint2);

    if(typeCreation == Tool::FromGui){
        AddToFile();
    }
}

void VModelingSpline::setDialog(){
    Q_ASSERT(!dialogSpline.isNull());
    if(!dialogSpline.isNull()){
        VSpline spl = VAbstractTool::data.GetModelingSpline(id);
        dialogSpline->setP1(spl.GetP1());
        dialogSpline->setP4(spl.GetP4());
        dialogSpline->setAngle1(spl.GetAngle1());
        dialogSpline->setAngle2(spl.GetAngle2());
        dialogSpline->setKAsm1(spl.GetKasm1());
        dialogSpline->setKAsm2(spl.GetKasm2());
        dialogSpline->setKCurve(spl.GetKcurve());
    }
}

VModelingSpline *VModelingSpline::Create(QSharedPointer<DialogSpline> &dialog, VDomDocument *doc,
                         VContainer *data){
    qint64 p1 = dialog->getP1();
    qint64 p4 = dialog->getP4();
    qreal kAsm1 = dialog->getKAsm1();
    qreal kAsm2 = dialog->getKAsm2();
    qreal angle1 = dialog->getAngle1();
    qreal angle2 = dialog->getAngle2();
    qreal kCurve = dialog->getKCurve();
    return Create(0, p1, p4, kAsm1, kAsm2, angle1, angle2, kCurve, doc, data, Document::FullParse,
                  Tool::FromGui);
}

VModelingSpline *VModelingSpline::Create(const qint64 _id, const qint64 &p1, const qint64 &p4,
                                         const qreal &kAsm1, const qreal kAsm2, const qreal &angle1,
                                         const qreal &angle2, const qreal &kCurve, VDomDocument *doc,
                                         VContainer *data, Document::Enum parse, Tool::Enum typeCreation){
    VModelingSpline *spl = 0;
    VSpline spline = VSpline(data->DataModelingPoints(), p1, p4, angle1, angle2, kAsm1, kAsm2, kCurve);
    qint64 id = _id;
    if(typeCreation == Tool::FromGui){
        id = data->AddModelingSpline(spline);
    } else {
        data->UpdateModelingSpline(id, spline);
        if(parse != Document::FullParse){
            QMap<qint64, VDataTool*>* tools = doc->getTools();
            VDataTool *tool = tools->value(id);
            Q_CHECK_PTR(tool);
            tool->VDataTool::setData(data);
            data->IncrementReferens(id, Scene::Spline, Draw::Modeling);
        }
    }
    data->AddLengthSpline(data->GetNameSpline(p1, p4, Draw::Modeling), spline.GetLength());
    data->IncrementReferens(p1, Scene::Point, Draw::Modeling);
    data->IncrementReferens(p4, Scene::Point, Draw::Modeling);
    if(parse == Document::FullParse){
        spl = new VModelingSpline(doc, data, id, typeCreation);
        QMap<qint64, VDataTool*>* tools = doc->getTools();
        tools->insert(id,spl);
    }
    return spl;
}

void VModelingSpline::FullUpdateFromFile(){
    RefreshGeometry();
}

void VModelingSpline::FullUpdateFromGui(int result){
    if(result == QDialog::Accepted){
        VSpline spl = VSpline (VAbstractTool::data.DataModelingPoints(), dialogSpline->getP1(),
                               dialogSpline->getP4(), dialogSpline->getAngle1(), dialogSpline->getAngle2(),
                               dialogSpline->getKAsm1(), dialogSpline->getKAsm2(), dialogSpline->getKCurve());

        disconnect(controlPoints[0], &VControlPointSpline::ControlPointChangePosition, this,
                &VModelingSpline::ControlPointChangePosition);
        disconnect(controlPoints[1], &VControlPointSpline::ControlPointChangePosition, this,
                &VModelingSpline::ControlPointChangePosition);
        controlPoints[0]->setPos(spl.GetP2());
        controlPoints[1]->setPos(spl.GetP3());
        connect(controlPoints[0], &VControlPointSpline::ControlPointChangePosition, this,
                &VModelingSpline::ControlPointChangePosition);
        connect(controlPoints[1], &VControlPointSpline::ControlPointChangePosition, this,
                &VModelingSpline::ControlPointChangePosition);

        spl = VSpline (VAbstractTool::data.DataModelingPoints(), dialogSpline->getP1(),
                       controlPoints[0]->pos(), controlPoints[1]->pos(), dialogSpline->getP4(),
                       dialogSpline->getKCurve());
        QDomElement domElement = doc->elementById(QString().setNum(id));
        if(domElement.isElement()){
            domElement.setAttribute("point1", QString().setNum(spl.GetP1()));
            domElement.setAttribute("point4", QString().setNum(spl.GetP4()));
            domElement.setAttribute("angle1", QString().setNum(spl.GetAngle1()));
            domElement.setAttribute("angle2", QString().setNum(spl.GetAngle2()));
            domElement.setAttribute("kAsm1", QString().setNum(spl.GetKasm1()));
            domElement.setAttribute("kAsm2", QString().setNum(spl.GetKasm2()));
            domElement.setAttribute("kCurve", QString().setNum(spl.GetKcurve()));
            emit FullUpdateTree();
        }
    }
    dialogSpline.clear();
}

void VModelingSpline::ControlPointChangePosition(const qint32 &indexSpline, SplinePoint::Position position,
                                              const QPointF pos){
    Q_UNUSED(indexSpline);
    VSpline spl = VAbstractTool::data.GetModelingSpline(id);
    if(position == SplinePoint::FirstPoint){
        spl.ModifiSpl (spl.GetP1(), pos, spl.GetP3(), spl.GetP4(), spl.GetKcurve());
    } else {
        spl.ModifiSpl (spl.GetP1(), spl.GetP2(), pos, spl.GetP4(), spl.GetKcurve());
    }
    QDomElement domElement = doc->elementById(QString().setNum(id));
    if(domElement.isElement()){
        domElement.setAttribute("angle1", QString().setNum(spl.GetAngle1()));
        domElement.setAttribute("angle2", QString().setNum(spl.GetAngle2()));
        domElement.setAttribute("kAsm1", QString().setNum(spl.GetKasm1()));
        domElement.setAttribute("kAsm2", QString().setNum(spl.GetKasm2()));
        domElement.setAttribute("kCurve", QString().setNum(spl.GetKcurve()));
        emit FullUpdateTree();
    }
}

void VModelingSpline::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
    ContextMenu(dialogSpline, this, event);
}

void VModelingSpline::AddToFile(){
    VSpline spl = VAbstractTool::data.GetModelingSpline(id);
    QDomElement domElement = doc->createElement("spline");

    AddAttribute(domElement, "id", id);
    AddAttribute(domElement, "type", "simple");
    AddAttribute(domElement, "point1", spl.GetP1());
    AddAttribute(domElement, "point4", spl.GetP4());
    AddAttribute(domElement, "angle1", spl.GetAngle1());
    AddAttribute(domElement, "angle2", spl.GetAngle2());
    AddAttribute(domElement, "kAsm1", spl.GetKasm1());
    AddAttribute(domElement, "kAsm2", spl.GetKasm2());
    AddAttribute(domElement, "kCurve", spl.GetKcurve());

    AddToModeling(domElement);
}

void VModelingSpline::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        emit ChoosedTool(id, Scene::Spline);
    }
    QGraphicsItem::mouseReleaseEvent(event);
}

void VModelingSpline::hoverMoveEvent(QGraphicsSceneHoverEvent *event){
    Q_UNUSED(event);
    this->setPen(QPen(currentColor, widthMainLine));
}

void VModelingSpline::hoverLeaveEvent(QGraphicsSceneHoverEvent *event){
    Q_UNUSED(event);
    this->setPen(QPen(currentColor, widthHairLine));
}

void VModelingSpline::RefreshGeometry(){
    VSpline spl = VAbstractTool::data.GetModelingSpline(id);
    QPainterPath path;
    path.addPath(spl.GetPath());
    path.setFillRule( Qt::WindingFill );
    this->setPath(path);
    QPointF splinePoint = VAbstractTool::data.GetModelingPoint(spl.GetP1()).toQPointF();
    QPointF controlPoint = spl.GetP2();
    emit RefreshLine(1, SplinePoint::FirstPoint, controlPoint, splinePoint);
    splinePoint = VAbstractTool::data.GetModelingPoint(spl.GetP4()).toQPointF();
    controlPoint = spl.GetP3();
    emit RefreshLine(1, SplinePoint::LastPoint, controlPoint, splinePoint);

    disconnect(controlPoints[0], &VControlPointSpline::ControlPointChangePosition, this,
            &VModelingSpline::ControlPointChangePosition);
    disconnect(controlPoints[1], &VControlPointSpline::ControlPointChangePosition, this,
            &VModelingSpline::ControlPointChangePosition);
    controlPoints[0]->setPos(spl.GetP2());
    controlPoints[1]->setPos(spl.GetP3());
    connect(controlPoints[0], &VControlPointSpline::ControlPointChangePosition, this,
            &VModelingSpline::ControlPointChangePosition);
    connect(controlPoints[1], &VControlPointSpline::ControlPointChangePosition, this,
            &VModelingSpline::ControlPointChangePosition);
}